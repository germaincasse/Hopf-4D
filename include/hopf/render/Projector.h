#pragma once

#include "hopf/geometry/Mesh4D.h"
#include "hopf/math/Mat5.h"

#include <cstdint>
#include <vector>

namespace hopf::render {

struct ProjectedMesh {
    struct Vertex3 {
        float x, y, z;
        float wDepth;
    };
    std::vector<Vertex3>  positions;
    std::vector<uint32_t> lineIndices;
    std::vector<uint32_t> triangleIndices;
};

// Parallel projection drops w; perspective scales xyz by focal4 / (w + wOffset).
ProjectedMesh projectMesh(const geometry::Mesh4D& mesh,
                          const math::Mat5& worldFromLocal,
                          bool perspective,
                          float focal4,
                          float wOffset);

} // namespace hopf::render
