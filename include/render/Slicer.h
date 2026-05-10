#pragma once

#include "geometry/Mesh4D.h"
#include "math/Mat5.h"

#include <cstdint>
#include <vector>

namespace hopf::render {

// Output of slicing a 4D mesh: non-indexed triangle list (3 vertices per triangle).
// Each vertex carries the triangle's outward normal (flat-shaded); identical for all
// three vertices of the triangle so flat interpolation is exact.
struct SliceMesh {
    struct TriVertex {
        float x, y, z;
        float nx, ny, nz;
    };
    std::vector<TriVertex> triVertices;
};

// Marching-tetrahedra intersection of a 4D mesh with the hyperplane
// (sliceAxis-th coordinate = sliceVal). The result lives in the 3D subspace orthogonal
// to sliceAxis (the three remaining axes, in axis order, become 3D (x, y, z)).
// Triangle normals are oriented outward from the slice solid's centroid.
//   sliceAxis: 0=X, 1=Y, 2=Z, 3=W
SliceMesh sliceMesh(const geometry::Mesh4D& mesh,
                    const math::Mat5& worldFromLocal,
                    int   sliceAxis,
                    float sliceVal);

} // namespace hopf::render
