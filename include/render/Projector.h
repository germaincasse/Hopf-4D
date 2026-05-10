#pragma once

#include "geometry/Mesh4D.h"
#include "math/Mat5.h"

#include <cstdint>
#include <vector>

namespace hopf::render {

struct ProjectedMesh {
    // Indexed line vertices for the wireframe styles.
    struct LineVertex {
        float x, y, z;
        float wDepth;
    };
    std::vector<LineVertex> linePositions;
    std::vector<uint32_t>   lineIndices;

    // Non-indexed triangle vertices for the solid styles. Each triangle has its outward
    // normal repeated on its 3 vertices (flat-shaded).
    struct TriVertex {
        float x, y, z;
        float nx, ny, nz;
    };
    std::vector<TriVertex> triVertices;
};

// Parallel projection drops w; perspective scales xyz by focal4 / (w + wOffset).
// Triangle normals are computed in 3D and oriented outward from the projected centroid.
ProjectedMesh projectMesh(const geometry::Mesh4D& mesh,
                          const math::Mat5& worldFromLocal,
                          bool perspective,
                          float focal4,
                          float wOffset);

} // namespace hopf::render
