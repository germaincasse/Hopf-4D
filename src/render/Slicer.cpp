#include "render/Slicer.h"

#include <array>
#include <cmath>

namespace hopf::render {

namespace {

struct V3 { float x, y, z; };

V3 interpDrop(const math::Vec4& a, const math::Vec4& b, float t, int sliceAxis) {
    const float comps[4] = {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t,
    };
    float out[3];
    int oi = 0;
    for (int i = 0; i < 4; ++i) {
        if (i != sliceAxis) out[oi++] = comps[i];
    }
    return { out[0], out[1], out[2] };
}

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

void pushTri(std::vector<SliceMesh::TriVertex>& out,
             V3 a, V3 b, V3 c, V3 centroid)
{
    V3 n = cross(sub(b, a), sub(c, a));
    const V3 triCenter = mul(add(add(a, b), c), 1.f / 3.f);
    if (dot(n, sub(triCenter, centroid)) < 0.f) n = { -n.x, -n.y, -n.z };
    n = normalize(n);
    out.push_back({ a.x, a.y, a.z, n.x, n.y, n.z });
    out.push_back({ b.x, b.y, b.z, n.x, n.y, n.z });
    out.push_back({ c.x, c.y, c.z, n.x, n.y, n.z });
}

} // namespace

SliceMesh sliceMesh(const geometry::Mesh4D& mesh,
                    const math::Mat5& worldFromLocal,
                    int   sliceAxis,
                    float sliceVal)
{
    SliceMesh out;

    std::vector<math::Vec4> world;
    world.reserve(mesh.vertices.size());
    for (const auto& v : mesh.vertices) {
        world.push_back(worldFromLocal.transformPoint(v));
    }

    // First pass: collect raw triangle vertices to compute the slice centroid.
    std::vector<V3> rawTris;
    rawTris.reserve(mesh.tetrahedra.size() * 3);

    for (const auto& tet : mesh.tetrahedra) {
        const math::Vec4 v[4] = {
            world[tet[0]], world[tet[1]], world[tet[2]], world[tet[3]],
        };
        const float d[4] = {
            v[0][sliceAxis] - sliceVal,
            v[1][sliceAxis] - sliceVal,
            v[2][sliceAxis] - sliceVal,
            v[3][sliceAxis] - sliceVal,
        };
        int pos[4]{}, neg[4]{};
        int np = 0, nn = 0;
        for (int i = 0; i < 4; ++i) {
            if (d[i] >= 0.f) pos[np++] = i;
            else             neg[nn++] = i;
        }
        if (np == 0 || nn == 0) continue;

        auto interp = [&](int a, int b) {
            const float denom = d[a] - d[b];
            const float t = denom != 0.f ? d[a] / denom : 0.f;
            return interpDrop(v[a], v[b], t, sliceAxis);
        };

        if (np == 1 || nn == 1) {
            const int s = (np == 1) ? pos[0] : neg[0];
            int o[3], oi = 0;
            for (int i = 0; i < 4; ++i) if (i != s) o[oi++] = i;
            rawTris.push_back(interp(s, o[0]));
            rawTris.push_back(interp(s, o[1]));
            rawTris.push_back(interp(s, o[2]));
        } else {
            const int p0 = pos[0], p1 = pos[1], n0 = neg[0], n1 = neg[1];
            const V3 q0 = interp(p0, n0);
            const V3 q1 = interp(p0, n1);
            const V3 q2 = interp(p1, n1);
            const V3 q3 = interp(p1, n0);
            rawTris.push_back(q0); rawTris.push_back(q1); rawTris.push_back(q2);
            rawTris.push_back(q0); rawTris.push_back(q2); rawTris.push_back(q3);
        }
    }

    if (rawTris.empty()) return out;

    V3 centroid{ 0.f, 0.f, 0.f };
    for (const auto& p : rawTris) {
        centroid.x += p.x; centroid.y += p.y; centroid.z += p.z;
    }
    const float inv = 1.f / float(rawTris.size());
    centroid.x *= inv; centroid.y *= inv; centroid.z *= inv;

    out.triVertices.reserve(rawTris.size());
    for (size_t i = 0; i + 2 < rawTris.size(); i += 3) {
        pushTri(out.triVertices, rawTris[i], rawTris[i + 1], rawTris[i + 2], centroid);
    }
    return out;
}

} // namespace hopf::render
