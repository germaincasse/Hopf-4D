#include "hopf/render/Slicer.h"

namespace hopf::render {

namespace {
inline SliceMesh::Vertex3 lerp3(const math::Vec4& a, const math::Vec4& b, float t) {
    return {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
    };
}
} // namespace

SliceMesh sliceMesh(const geometry::Mesh4D& mesh,
                    const math::Mat5& worldFromLocal,
                    float sliceW)
{
    SliceMesh out;

    // Pre-transform all mesh vertices to world space once.
    std::vector<math::Vec4> world;
    world.reserve(mesh.vertices.size());
    for (const auto& v : mesh.vertices) {
        world.push_back(worldFromLocal.transformPoint(v));
    }

    out.positions.reserve(mesh.tetrahedra.size() * 2);
    out.indices.reserve(mesh.tetrahedra.size() * 6);

    for (const auto& tet : mesh.tetrahedra) {
        const math::Vec4 v[4] = {
            world[tet[0]], world[tet[1]], world[tet[2]], world[tet[3]],
        };
        const float d[4] = {
            v[0].w - sliceW,
            v[1].w - sliceW,
            v[2].w - sliceW,
            v[3].w - sliceW,
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
            return lerp3(v[a], v[b], t);
        };

        if (np == 1 || nn == 1) {
            // Triangle: 3 intersection points (between the singleton and the other 3).
            const int s = (np == 1) ? pos[0] : neg[0];
            int o[3], oi = 0;
            for (int i = 0; i < 4; ++i) if (i != s) o[oi++] = i;

            const uint32_t base = static_cast<uint32_t>(out.positions.size());
            out.positions.push_back(interp(s, o[0]));
            out.positions.push_back(interp(s, o[1]));
            out.positions.push_back(interp(s, o[2]));
            out.indices.push_back(base + 0);
            out.indices.push_back(base + 1);
            out.indices.push_back(base + 2);
        } else {
            // Quadrilateral: 4 intersection points along edges (p0,n0), (p0,n1), (p1,n1), (p1,n0).
            const int p0 = pos[0], p1 = pos[1], n0 = neg[0], n1 = neg[1];
            const uint32_t base = static_cast<uint32_t>(out.positions.size());
            out.positions.push_back(interp(p0, n0));
            out.positions.push_back(interp(p0, n1));
            out.positions.push_back(interp(p1, n1));
            out.positions.push_back(interp(p1, n0));
            out.indices.push_back(base + 0);
            out.indices.push_back(base + 1);
            out.indices.push_back(base + 2);
            out.indices.push_back(base + 0);
            out.indices.push_back(base + 2);
            out.indices.push_back(base + 3);
        }
    }

    return out;
}

} // namespace hopf::render
