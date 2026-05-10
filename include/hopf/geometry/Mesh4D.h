#pragma once

#include "hopf/math/Vec4.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace hopf::geometry {

// Canonical 4D mesh container. Cells (3D-bounded volumes) are stored as tetrahedra so the
// slicer can stay simple — non-tetrahedral cells must be triangulated at build time.
struct Mesh4D {
    std::vector<math::Vec4>              vertices;
    std::vector<std::array<uint32_t, 2>> edges;
    std::vector<std::array<uint32_t, 3>> triangles;
    std::vector<std::array<uint32_t, 4>> tetrahedra;

    std::string name;
};

} // namespace hopf::geometry
