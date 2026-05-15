#include "physics/Physics.h"

#include "math/Bivector4.h"
#include "math/Vec4.h"
#include "scene/Entity.h"
#include "scene/Scene.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace hopf::physics {

namespace {

constexpr float kGravityY        = -9.81f;
constexpr float kAngularDamping  = 0.20f;
constexpr float kLinearDamping   = 0.02f;
constexpr float kFrictionCoef    = 0.5f;
constexpr float kRestitution     = 0.0f;
constexpr float kPositionSlop    = 5e-3f;   // contacts within slop don't get corrected
constexpr float kBaumgarte       = 0.2f;    // fraction of penetration corrected per frame
constexpr int   kSolverIters     = 4;

// Oriented bounding box derived from the entity's mesh AABB and Transform4D.
// halfExtents are in the body's local axes (scaled), R rotates local to world.
struct OBB {
    math::Vec4 center{};
    math::Vec4 halfExtents{};
    math::Mat5 R{ math::Mat5::identity() };
};

float dot4(const math::Vec4& a, const math::Vec4& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

math::Vec4 axisOf(const OBB& obb, int i) {
    math::Vec4 e{};
    e[i] = 1.f;
    return obb.R.transformDirection(e);
}

// Local-space AABB of the mesh, with a fallback to a unit cube if missing.
void localAABB(const scene::Entity& e, math::Vec4& mn, math::Vec4& mx) {
    constexpr float kHuge = std::numeric_limits<float>::max();
    mn = { kHuge,  kHuge,  kHuge,  kHuge};
    mx = {-kHuge, -kHuge, -kHuge, -kHuge};
    if (e.mesh && !e.mesh->vertices.empty()) {
        for (const auto& v : e.mesh->vertices) {
            for (int i = 0; i < 4; ++i) {
                if (v[i] < mn[i]) mn[i] = v[i];
                if (v[i] > mx[i]) mx[i] = v[i];
            }
        }
    } else {
        mn = {-0.5f, -0.5f, -0.5f, -0.5f};
        mx = { 0.5f,  0.5f,  0.5f,  0.5f};
    }
}

OBB computeOBB(const scene::Entity& e) {
    OBB obb;
    math::Vec4 mn, mx;
    localAABB(e, mn, mx);
    const math::Vec4 localCenter{(mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f,
                                 (mn.z + mx.z) * 0.5f, (mn.w + mx.w) * 0.5f};
    obb.halfExtents = {(mx.x - mn.x) * 0.5f * e.transform.scale.x,
                       (mx.y - mn.y) * 0.5f * e.transform.scale.y,
                       (mx.z - mn.z) * 0.5f * e.transform.scale.z,
                       (mx.w - mn.w) * 0.5f * e.transform.scale.w};
    obb.R = e.transform.rotation.toMatrix();
    const math::Vec4 scaledCenter{localCenter.x * e.transform.scale.x,
                                  localCenter.y * e.transform.scale.y,
                                  localCenter.z * e.transform.scale.z,
                                  localCenter.w * e.transform.scale.w};
    obb.center = e.transform.position + obb.R.transformDirection(scaledCenter);
    return obb;
}

// World-axis-aligned half-extent of an OBB along world axis k.
// = sum over local axes i of |R[i][k]| * halfExtents[i].
float worldExtentAlong(const OBB& obb, int k, int dim) {
    float e = 0.f;
    for (int i = 0; i < dim; ++i) {
        const math::Vec4 ax = axisOf(obb, i);
        e += std::abs(ax[k]) * obb.halfExtents[i];
    }
    return e;
}

// Effective center-of-pressure between two contacting bodies, projected onto the
// contact plane and clipped to the overlap rectangle. This models a stable resting
// pair: as long as the body's COM projects inside the support footprint, the contact
// sits exactly under it (no torque, no tipping). The moment the COM projection
// crosses the support edge, the contact gets clamped to that edge, the lever arm
// becomes non-zero and the body starts tipping --- which is exactly how center-of-
// gravity stability works for rigid bodies in real life.
//
// The "preferred" pre-clamp position is weighted toward the more dynamic body
// (higher inverse mass); a kinematic ground steers the contact entirely under the
// dynamic body's COM.
math::Vec4 contactPoint(const OBB& A, const OBB& B,
                        const math::Vec4& normal, int dim,
                        float wa, float wb)
{
    int normalAxis = 0;
    float bestAbs = std::abs(normal[0]);
    for (int k = 1; k < dim; ++k) {
        const float a = std::abs(normal[k]);
        if (a > bestAbs) { bestAbs = a; normalAxis = k; }
    }

    const float wSum = wa + wb;
    auto preferred = [&](int k) -> float {
        if (wSum <= 0.f) return (A.center[k] + B.center[k]) * 0.5f;
        return (wa * A.center[k] + wb * B.center[k]) / wSum;
    };

    math::Vec4 c{};
    for (int k = 0; k < dim; ++k) {
        const float eA = worldExtentAlong(A, k, dim);
        const float eB = worldExtentAlong(B, k, dim);
        if (k == normalAxis) {
            const float sign = (normal[k] >= 0.f) ? 1.f : -1.f;
            const float aFace = A.center[k] - sign * eA;
            const float bFace = B.center[k] + sign * eB;
            c[k] = (aFace + bFace) * 0.5f;
        } else {
            const float lo = std::max(A.center[k] - eA, B.center[k] - eB);
            const float hi = std::min(A.center[k] + eA, B.center[k] + eB);
            c[k] = std::clamp(preferred(k), lo, hi);
        }
    }
    return c;
}

// SAT between two OBBs using face normals only (2 * dim axes). For 2D this is exact;
// for 3D and 4D it can miss edge-edge separating axes, but it is sufficient for the
// face/face and face/vertex configurations dominant in stacking and dropping scenarios.
bool satCollide(const OBB& A, const OBB& B, int dim,
                math::Vec4& outNormal, float& outPen)
{
    float minPen = std::numeric_limits<float>::max();
    math::Vec4 bestL{};

    auto test = [&](math::Vec4 L) -> bool {
        const float lenSq = dot4(L, L);
        if (lenSq < 1e-10f) return true;
        L = L * (1.f / std::sqrt(lenSq));

        float rA = 0.f, rB = 0.f;
        for (int i = 0; i < dim; ++i) {
            rA += A.halfExtents[i] * std::abs(dot4(L, axisOf(A, i)));
            rB += B.halfExtents[i] * std::abs(dot4(L, axisOf(B, i)));
        }
        const math::Vec4 diff = A.center - B.center;
        const float dist     = dot4(L, diff);
        const float absDist  = std::abs(dist);
        const float overlap  = (rA + rB) - absDist;
        if (overlap <= 0.f) return false;
        if (overlap < minPen) {
            minPen = overlap;
            bestL = (dist >= 0.f) ? L : math::Vec4{-L.x, -L.y, -L.z, -L.w};
        }
        return true;
    };

    for (int i = 0; i < dim; ++i) {
        if (!test(axisOf(A, i))) return false;
        if (!test(axisOf(B, i))) return false;
    }

    outNormal = bestL;
    outPen    = minPen;
    return true;
}

float wedgeSqNorm(const math::Bivector4& v) {
    return v.xy * v.xy + v.xz * v.xz + v.xw * v.xw
         + v.yz * v.yz + v.yw * v.yw + v.zw * v.zw;
}

float computeInertia(const OBB& obb, float mass, int dim) {
    if (mass <= 0.f) return 0.f;
    float sum = 0.f;
    for (int i = 0; i < dim; ++i) {
        const float e = obb.halfExtents[i] * 2.f;
        sum += e * e;
    }
    return std::max(1e-4f, mass * sum / 12.f);
}

void addBivector(math::Bivector4& a, const math::Bivector4& b, float s) {
    a.xy += b.xy * s; a.xz += b.xz * s; a.xw += b.xw * s;
    a.yz += b.yz * s; a.yw += b.yw * s; a.zw += b.zw * s;
}

void resolvePair(scene::Entity& A, scene::Entity& B, int dim, float dt) {
    const OBB oA = computeOBB(A);
    const OBB oB = computeOBB(B);
    math::Vec4 normal{};
    float pen = 0.f;
    if (!satCollide(oA, oB, dim, normal, pen)) return;

    auto invMass = [](const scene::Entity& e) -> float {
        if (!e.rigidbody.has_value() || e.rigidbody->kinematic) return 0.f;
        return 1.f / std::max(0.001f, e.rigidbody->mass);
    };
    const float wa = invMass(A);
    const float wb = invMass(B);
    const float wSum = wa + wb;
    if (wSum <= 0.f) return;

    const math::Vec4 contact = contactPoint(oA, oB, normal, dim, wa, wb);
    const math::Vec4 ra = contact - A.transform.position;
    const math::Vec4 rb = contact - B.transform.position;

    const float iA = (wa > 0.f) ? 1.f / computeInertia(oA, A.rigidbody->mass, dim) : 0.f;
    const float iB = (wb > 0.f) ? 1.f / computeInertia(oB, B.rigidbody->mass, dim) : 0.f;

    auto pointVel = [](const scene::Entity& e, const math::Vec4& r) -> math::Vec4 {
        if (!e.rigidbody.has_value()) return {};
        return e.rigidbody->velocity + math::velocityAt(e.rigidbody->angularVelocity, r);
    };

    const math::Vec4 vA = pointVel(A, ra);
    const math::Vec4 vB = pointVel(B, rb);
    const math::Vec4 vRel = vA - vB;
    const float relNormal = dot4(vRel, normal);

    // Baumgarte stabilization: instead of teleporting positions, we add a small
    // separating bias velocity proportional to the excess penetration. This nudges
    // bodies apart over a few frames without injecting energy.
    const float bias = (pen > kPositionSlop)
                        ? (kBaumgarte / std::max(dt, 1e-6f)) * (pen - kPositionSlop)
                        : 0.f;

    // Already separating (and not penetrating beyond slop): nothing to do.
    if (relNormal >= 0.f && bias <= 0.f) return;

    const math::Bivector4 raCrossN = math::wedge(ra, normal);
    const math::Bivector4 rbCrossN = math::wedge(rb, normal);
    const float denomN = wa + wb + iA * wedgeSqNorm(raCrossN) + iB * wedgeSqNorm(rbCrossN);
    if (denomN <= 0.f) return;

    // Restitution kicks in only when the approach velocity is significant; this
    // prevents the body from "buzzing" against a floor under gravity.
    const float restitutionV = (relNormal < -1.f) ? kRestitution : 0.f;
    float j = (-(1.f + restitutionV) * relNormal + bias) / denomN;
    if (j < 0.f) j = 0.f; // never pull bodies together
    const math::Vec4 J{normal.x * j, normal.y * j, normal.z * j, normal.w * j};
    if (wa > 0.f) {
        A.rigidbody->velocity.x += J.x * wa;
        A.rigidbody->velocity.y += J.y * wa;
        A.rigidbody->velocity.z += J.z * wa;
        A.rigidbody->velocity.w += J.w * wa;
        addBivector(A.rigidbody->angularVelocity, math::wedge(ra, J), iA);
    }
    if (wb > 0.f) {
        B.rigidbody->velocity.x -= J.x * wb;
        B.rigidbody->velocity.y -= J.y * wb;
        B.rigidbody->velocity.z -= J.z * wb;
        B.rigidbody->velocity.w -= J.w * wb;
        addBivector(B.rigidbody->angularVelocity, math::wedge(rb, math::Vec4{-J.x, -J.y, -J.z, -J.w}), iB);
    }

    if (kFrictionCoef > 0.f && std::abs(j) > 1e-6f) {
        const math::Vec4 vA2 = pointVel(A, ra);
        const math::Vec4 vB2 = pointVel(B, rb);
        const math::Vec4 vRel2{vA2.x - vB2.x, vA2.y - vB2.y, vA2.z - vB2.z, vA2.w - vB2.w};
        const float vNorm2 = dot4(vRel2, normal);
        const math::Vec4 vTan{vRel2.x - vNorm2 * normal.x,
                              vRel2.y - vNorm2 * normal.y,
                              vRel2.z - vNorm2 * normal.z,
                              vRel2.w - vNorm2 * normal.w};
        const float vTanLen = std::sqrt(dot4(vTan, vTan));
        if (vTanLen > 1e-5f) {
            const math::Vec4 t{vTan.x / vTanLen, vTan.y / vTanLen,
                               vTan.z / vTanLen, vTan.w / vTanLen};
            const math::Bivector4 raCrossT = math::wedge(ra, t);
            const math::Bivector4 rbCrossT = math::wedge(rb, t);
            const float denomT = wa + wb + iA * wedgeSqNorm(raCrossT) + iB * wedgeSqNorm(rbCrossT);
            float jt = denomT > 0.f ? -vTanLen / denomT : 0.f;
            const float clamp = std::abs(j) * kFrictionCoef;
            jt = std::clamp(jt, -clamp, clamp);
            const math::Vec4 Jt{t.x * jt, t.y * jt, t.z * jt, t.w * jt};
            if (wa > 0.f) {
                A.rigidbody->velocity.x += Jt.x * wa;
                A.rigidbody->velocity.y += Jt.y * wa;
                A.rigidbody->velocity.z += Jt.z * wa;
                A.rigidbody->velocity.w += Jt.w * wa;
                addBivector(A.rigidbody->angularVelocity, math::wedge(ra, Jt), iA);
            }
            if (wb > 0.f) {
                B.rigidbody->velocity.x -= Jt.x * wb;
                B.rigidbody->velocity.y -= Jt.y * wb;
                B.rigidbody->velocity.z -= Jt.z * wb;
                B.rigidbody->velocity.w -= Jt.w * wb;
                addBivector(B.rigidbody->angularVelocity,
                            math::wedge(rb, math::Vec4{-Jt.x, -Jt.y, -Jt.z, -Jt.w}), iB);
            }
        }
    }
}

} // namespace

void step(scene::Scene& scene, float dt) {
    auto& ents = scene.entities();

    for (auto& e : ents) {
        e.transform.rotation.xy += e.autoRotate.xy * dt;
        e.transform.rotation.xz += e.autoRotate.xz * dt;
        e.transform.rotation.xw += e.autoRotate.xw * dt;
        e.transform.rotation.yz += e.autoRotate.yz * dt;
        e.transform.rotation.yw += e.autoRotate.yw * dt;
        e.transform.rotation.zw += e.autoRotate.zw * dt;

        if (!e.rigidbody.has_value() || e.rigidbody->kinematic) continue;

        if (e.rigidbody->useGravity) {
            e.rigidbody->velocity.y += kGravityY * dt;
        }
        if constexpr (kLinearDamping > 0.f) {
            const float k = std::max(0.f, 1.f - kLinearDamping * dt);
            e.rigidbody->velocity.x *= k;
            e.rigidbody->velocity.y *= k;
            e.rigidbody->velocity.z *= k;
            e.rigidbody->velocity.w *= k;
        }
        if constexpr (kAngularDamping > 0.f) {
            const float k = std::max(0.f, 1.f - kAngularDamping * dt);
            e.rigidbody->angularVelocity.xy *= k;
            e.rigidbody->angularVelocity.xz *= k;
            e.rigidbody->angularVelocity.xw *= k;
            e.rigidbody->angularVelocity.yz *= k;
            e.rigidbody->angularVelocity.yw *= k;
            e.rigidbody->angularVelocity.zw *= k;
        }

        e.transform.position.x += e.rigidbody->velocity.x * dt;
        e.transform.position.y += e.rigidbody->velocity.y * dt;
        e.transform.position.z += e.rigidbody->velocity.z * dt;
        e.transform.position.w += e.rigidbody->velocity.w * dt;

        e.transform.rotation.xy += e.rigidbody->angularVelocity.xy * dt;
        e.transform.rotation.xz += e.rigidbody->angularVelocity.xz * dt;
        e.transform.rotation.xw += e.rigidbody->angularVelocity.xw * dt;
        e.transform.rotation.yz += e.rigidbody->angularVelocity.yz * dt;
        e.transform.rotation.yw += e.rigidbody->angularVelocity.yw * dt;
        e.transform.rotation.zw += e.rigidbody->angularVelocity.zw * dt;
    }

    // Sequential impulses: run several relaxation passes for stable stacks.
    for (int iter = 0; iter < kSolverIters; ++iter) {
        for (size_t i = 0; i < ents.size(); ++i) {
            auto& A = ents[i];
            if (!A.collider.has_value() || A.collider->isTrigger) continue;
            for (size_t j = i + 1; j < ents.size(); ++j) {
                auto& B = ents[j];
                if (!B.collider.has_value() || B.collider->isTrigger) continue;
                if (A.collider->dimension != B.collider->dimension) continue;
                const bool aDyn = A.rigidbody.has_value() && !A.rigidbody->kinematic;
                const bool bDyn = B.rigidbody.has_value() && !B.rigidbody->kinematic;
                if (!aDyn && !bDyn) continue;
                resolvePair(A, B, A.collider->dimension, dt);
            }
        }
    }
}

} // namespace hopf::physics
