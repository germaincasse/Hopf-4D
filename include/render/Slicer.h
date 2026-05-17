#pragma once

#include "geometry/Mesh4D.h"
#include "math/Mat5.h"
#include "math/Vec4.h"

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

// Orthonormal 3D basis spanning the slice hyperplane (the subspace perpendicular to
// the slice normal). 4D points are mapped to 3D via the dot product with each basis
// vector. Use buildSliceBasis() to construct one from any unit normal.
struct SliceBasis {
    math::Vec4 e0{1, 0, 0, 0};
    math::Vec4 e1{0, 1, 0, 0};
    math::Vec4 e2{0, 0, 1, 0};
};

// Returns a normalized copy of v (or a fallback to +W when v is degenerate).
math::Vec4 normalizeSliceNormal(const math::Vec4& v);

// Builds three orthonormal vectors spanning the hyperplane perpendicular to `normal`.
// `normal` must be a unit Vec4 (use normalizeSliceNormal() first if unsure).
SliceBasis buildSliceBasis(const math::Vec4& normal);

// Project a 4D point onto the slice hyperplane's basis (3D output coords).
inline void projectOntoSliceBasis(const math::Vec4& p, const SliceBasis& b,
                                  float& x, float& y, float& z) {
    x = p.x * b.e0.x + p.y * b.e0.y + p.z * b.e0.z + p.w * b.e0.w;
    y = p.x * b.e1.x + p.y * b.e1.y + p.z * b.e1.z + p.w * b.e1.w;
    z = p.x * b.e2.x + p.y * b.e2.y + p.z * b.e2.z + p.w * b.e2.w;
}

// Marching-tetrahedra intersection of a 4D mesh with the hyperplane
//   { p : dot(p, sliceNormal) = sliceOffset }
// (where sliceNormal is a unit 4D vector). The 3D output lives in the basis
// returned by buildSliceBasis(sliceNormal). Triangle normals are oriented outward
// from the slice solid's centroid.
SliceMesh sliceMesh(const geometry::Mesh4D& mesh,
                    const math::Mat5& worldFromLocal,
                    const math::Vec4& sliceNormal,
                    float sliceOffset);

} // namespace hopf::render
