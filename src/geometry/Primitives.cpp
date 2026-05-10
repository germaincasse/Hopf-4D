#include "hopf/geometry/Primitives.h"

#include "PrimitivesInternal.h"

namespace hopf::geometry {

const char* primitiveLabel(PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Triangle2D:       return "Triangle";
        case PrimitiveType::Square2D:         return "Square";
        case PrimitiveType::Pentagon2D:       return "Pentagon";
        case PrimitiveType::Hexagon2D:        return "Hexagon";
        case PrimitiveType::Star2D:           return "Star";
        case PrimitiveType::Circle2D:         return "Circle";

        case PrimitiveType::Cube3D:           return "Cube";
        case PrimitiveType::Tetrahedron3D:    return "Tetrahedron";
        case PrimitiveType::Octahedron3D:     return "Octahedron";
        case PrimitiveType::Icosahedron3D:    return "Icosahedron";
        case PrimitiveType::Sphere3D:         return "Sphere";
        case PrimitiveType::Cylinder3D:       return "Cylinder";
        case PrimitiveType::Cone3D:           return "Cone";
        case PrimitiveType::Torus3D:          return "Torus";
        case PrimitiveType::Pyramid3D:        return "Pyramid";
        case PrimitiveType::Prism3D:          return "Prism";

        case PrimitiveType::Tesseract:        return "Tesseract";
        case PrimitiveType::Pentachoron:      return "Pentachoron (5-cell)";
        case PrimitiveType::Hexadecachoron:   return "Hexadecachoron (16-cell)";
        case PrimitiveType::Icositetrachoron: return "Icositetrachoron (24-cell)";
        case PrimitiveType::TetrahedralPrism: return "Tetrahedral prism";
        case PrimitiveType::CubicalPyramid:   return "Cubical pyramid";
    }
    return "Unknown";
}

bool primitiveIs2D(PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Triangle2D:
        case PrimitiveType::Square2D:
        case PrimitiveType::Pentagon2D:
        case PrimitiveType::Hexagon2D:
        case PrimitiveType::Star2D:
        case PrimitiveType::Circle2D:
            return true;
        default:
            return false;
    }
}

bool primitiveIs3D(PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Cube3D:
        case PrimitiveType::Tetrahedron3D:
        case PrimitiveType::Octahedron3D:
        case PrimitiveType::Icosahedron3D:
        case PrimitiveType::Sphere3D:
        case PrimitiveType::Cylinder3D:
        case PrimitiveType::Cone3D:
        case PrimitiveType::Torus3D:
        case PrimitiveType::Pyramid3D:
        case PrimitiveType::Prism3D:
            return true;
        default:
            return false;
    }
}

bool primitiveIs4D(PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Tesseract:
        case PrimitiveType::Pentachoron:
        case PrimitiveType::Hexadecachoron:
        case PrimitiveType::Icositetrachoron:
        case PrimitiveType::TetrahedralPrism:
        case PrimitiveType::CubicalPyramid:
            return true;
        default:
            return false;
    }
}

Mesh4D buildPrimitive(PrimitiveType type, float size) {
    switch (type) {
        case PrimitiveType::Triangle2D:       return detail::buildTriangle2D(size);
        case PrimitiveType::Square2D:         return detail::buildSquare2D(size);
        case PrimitiveType::Pentagon2D:       return detail::buildPentagon2D(size);
        case PrimitiveType::Hexagon2D:        return detail::buildHexagon2D(size);
        case PrimitiveType::Star2D:           return detail::buildStar2D(size);
        case PrimitiveType::Circle2D:         return detail::buildCircle2D(size);

        case PrimitiveType::Cube3D:           return detail::buildCube3D(size);
        case PrimitiveType::Tetrahedron3D:    return detail::buildTetrahedron3D(size);
        case PrimitiveType::Octahedron3D:     return detail::buildOctahedron3D(size);
        case PrimitiveType::Icosahedron3D:    return detail::buildIcosahedron3D(size);
        case PrimitiveType::Sphere3D:         return detail::buildSphere3D(size);
        case PrimitiveType::Cylinder3D:       return detail::buildCylinder3D(size);
        case PrimitiveType::Cone3D:           return detail::buildCone3D(size);
        case PrimitiveType::Torus3D:          return detail::buildTorus3D(size);
        case PrimitiveType::Pyramid3D:        return detail::buildPyramid3D(size);
        case PrimitiveType::Prism3D:          return detail::buildPrism3D(size);

        case PrimitiveType::Tesseract:        return detail::buildTesseract(size);
        case PrimitiveType::Pentachoron:      return detail::buildPentachoron(size);
        case PrimitiveType::Hexadecachoron:   return detail::buildHexadecachoron(size);
        case PrimitiveType::Icositetrachoron: return detail::buildIcositetrachoron(size);
        case PrimitiveType::TetrahedralPrism: return detail::buildTetrahedralPrism(size);
        case PrimitiveType::CubicalPyramid:   return detail::buildCubicalPyramid(size);
    }
    return {};
}

} // namespace hopf::geometry
