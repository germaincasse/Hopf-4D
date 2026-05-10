#include "render/Projector.h"

namespace hopf::render {

ProjectedMesh projectMesh(const geometry::Mesh4D& mesh,
                          const math::Mat5& worldFromLocal,
                          bool perspective,
                          float focal4,
                          float wOffset)
{
    ProjectedMesh out;
    out.positions.reserve(mesh.vertices.size());

    for (const auto& vLocal : mesh.vertices) {
        const math::Vec4 v = worldFromLocal.transformPoint(vLocal);
        ProjectedMesh::Vertex3 p3{};
        if (perspective) {
            const float wp = v.w + wOffset;
            const float k = wp != 0.f ? focal4 / wp : 1.f;
            p3 = { v.x * k, v.y * k, v.z * k, v.w };
        } else {
            p3 = { v.x, v.y, v.z, v.w };
        }
        out.positions.push_back(p3);
    }

    out.lineIndices.reserve(mesh.edges.size() * 2);
    for (const auto& e : mesh.edges) {
        out.lineIndices.push_back(e[0]);
        out.lineIndices.push_back(e[1]);
    }

    out.triangleIndices.reserve(mesh.triangles.size() * 3);
    for (const auto& t : mesh.triangles) {
        out.triangleIndices.push_back(t[0]);
        out.triangleIndices.push_back(t[1]);
        out.triangleIndices.push_back(t[2]);
    }
    return out;
}

} // namespace hopf::render
