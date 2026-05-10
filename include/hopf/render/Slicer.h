#pragma once

#include "hopf/geometry/Mesh4D.h"
#include "hopf/math/Mat5.h"

#include <cstdint>
#include <vector>

namespace hopf::render {

struct SliceMesh {
    struct Vertex3 { float x, y, z; };
    std::vector<Vertex3>  positions;
    std::vector<uint32_t> indices;
};

// Marching-tetrahedra intersection of a 4D mesh with the hyperplane w = sliceW.
SliceMesh sliceMesh(const geometry::Mesh4D& mesh,
                    const math::Mat5& worldFromLocal,
                    float sliceW);

} // namespace hopf::render
