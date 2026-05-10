#include "render/Projector.h"

#include <cmath>

namespace hopf::render {

namespace {

struct V3 { float x, y, z; };

inline V3 sub(V3 a, V3 b)  { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline V3 add(V3 a, V3 b)  { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline V3 mul(V3 a, float s){ return { a.x * s, a.y * s, a.z * s }; }
inline float dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline V3 cross(V3 a, V3 b) {
    return { a.y * b.z - a.z * b.y,
             a.z * b.x - a.x * b.z,
             a.x * b.y - a.y * b.x };
}
inline V3 normalize(V3 v) {
    const float l = std::sqrt(dot(v, v));
    return l > 1e-6f ? V3{ v.x / l, v.y / l, v.z / l } : V3{ 0, 0, 0 };
}

} // namespace

ProjectedMesh projectMesh(const geometry::Mesh4D& mesh,
                          const math::Mat5& worldFromLocal,
                          bool perspective,
                          float focal4,
                          float wOffset)
{
    ProjectedMesh out;
    out.linePositions.reserve(mesh.vertices.size());

    for (const auto& vLocal : mesh.vertices) {
        const math::Vec4 v = worldFromLocal.transformPoint(vLocal);
        ProjectedMesh::LineVertex lv{};
        if (perspective) {
            const float wp = v.w + wOffset;
            const float k = wp != 0.f ? focal4 / wp : 1.f;
            lv = { v.x * k, v.y * k, v.z * k, v.w };
        } else {
            lv = { v.x, v.y, v.z, v.w };
        }
        out.linePositions.push_back(lv);
    }

    out.lineIndices.reserve(mesh.edges.size() * 2);
    for (const auto& e : mesh.edges) {
        out.lineIndices.push_back(e[0]);
        out.lineIndices.push_back(e[1]);
    }

    if (mesh.triangles.empty()) return out;

    V3 centroid{ 0.f, 0.f, 0.f };
    for (const auto& v : out.linePositions) {
        centroid.x += v.x; centroid.y += v.y; centroid.z += v.z;
    }
    const float inv = 1.f / float(out.linePositions.size());
    centroid.x *= inv; centroid.y *= inv; centroid.z *= inv;

    out.triVertices.reserve(mesh.triangles.size() * 3);
    for (const auto& t : mesh.triangles) {
        const auto& va = out.linePositions[t[0]];
        const auto& vb = out.linePositions[t[1]];
        const auto& vc = out.linePositions[t[2]];
        const V3 a{ va.x, va.y, va.z };
        const V3 b{ vb.x, vb.y, vb.z };
        const V3 c{ vc.x, vc.y, vc.z };

        V3 n = cross(sub(b, a), sub(c, a));
        const V3 triCenter = mul(add(add(a, b), c), 1.f / 3.f);
        if (dot(n, sub(triCenter, centroid)) < 0.f) n = { -n.x, -n.y, -n.z };
        n = normalize(n);

        out.triVertices.push_back({ a.x, a.y, a.z, n.x, n.y, n.z });
        out.triVertices.push_back({ b.x, b.y, b.z, n.x, n.y, n.z });
        out.triVertices.push_back({ c.x, c.y, c.z, n.x, n.y, n.z });
    }
    return out;
}

} // namespace hopf::render
