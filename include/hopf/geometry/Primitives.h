#pragma once

#include "hopf/geometry/Mesh4D.h"

namespace hopf::geometry {

enum class PrimitiveType {
    // 2D (live at z = 0, w = 0)
    Triangle2D,
    Square2D,
    Pentagon2D,
    Hexagon2D,
    Star2D,
    Circle2D,

    // 3D (live at w = 0)
    Cube3D,
    Tetrahedron3D,
    Octahedron3D,
    Icosahedron3D,
    Sphere3D,
    Cylinder3D,
    Cone3D,
    Torus3D,
    Pyramid3D,
    Prism3D,

    // 4D
    Tesseract,
    Pentachoron,
    Hexadecachoron,
    Icositetrachoron,
    TetrahedralPrism,
    CubicalPyramid,
};

const char* primitiveLabel(PrimitiveType type);
bool        primitiveIs2D(PrimitiveType type);
bool        primitiveIs3D(PrimitiveType type);
bool        primitiveIs4D(PrimitiveType type);
Mesh4D      buildPrimitive(PrimitiveType type, float size = 1.f);

} // namespace hopf::geometry
