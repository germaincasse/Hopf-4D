#include <catch2/catch_test_macros.hpp>

#include "hopf/geometry/Primitives.h"

using namespace hopf::geometry;

TEST_CASE("Tesseract counts", "[geometry][tesseract]") {
    const Mesh4D m = buildPrimitive(PrimitiveType::Tesseract);
    REQUIRE(m.vertices.size()   == 16);
    REQUIRE(m.edges.size()      == 32);
    REQUIRE(m.triangles.size()  == 48);
    REQUIRE(m.tetrahedra.size() == 40);
}

TEST_CASE("Pentachoron is the 4-simplex", "[geometry][pentachoron]") {
    const Mesh4D m = buildPrimitive(PrimitiveType::Pentachoron);
    REQUIRE(m.vertices.size()   == 5);
    REQUIRE(m.edges.size()      == 10);
    REQUIRE(m.triangles.size()  == 10);
    REQUIRE(m.tetrahedra.size() == 5);
}

TEST_CASE("Hexadecachoron is the 4D cross-polytope", "[geometry][hexadecachoron]") {
    const Mesh4D m = buildPrimitive(PrimitiveType::Hexadecachoron);
    REQUIRE(m.vertices.size()   == 8);
    REQUIRE(m.edges.size()      == 24);
    REQUIRE(m.triangles.size()  == 32);
    REQUIRE(m.tetrahedra.size() == 16);
}

TEST_CASE("Icositetrachoron is the 24-cell", "[geometry][icositetrachoron]") {
    const Mesh4D m = buildPrimitive(PrimitiveType::Icositetrachoron);
    // 24 polytope vertices + 1 centroid for fan tetrahedrization.
    REQUIRE(m.vertices.size()   == 25);
    REQUIRE(m.edges.size()      == 96);
    REQUIRE(m.triangles.size()  == 96);
    REQUIRE(m.tetrahedra.size() == 96);
}

TEST_CASE("Tetrahedral prism counts", "[geometry][prism4d]") {
    const Mesh4D m = buildPrimitive(PrimitiveType::TetrahedralPrism);
    REQUIRE(m.vertices.size()   == 8);
    REQUIRE(m.edges.size()      == 16);
    REQUIRE(m.triangles.size()  == 20);
    REQUIRE(m.tetrahedra.size() == 14);
}

TEST_CASE("Cubical pyramid counts", "[geometry][pyramid4d]") {
    const Mesh4D m = buildPrimitive(PrimitiveType::CubicalPyramid);
    REQUIRE(m.vertices.size()   == 9);
    REQUIRE(m.edges.size()      == 20);
    REQUIRE(m.triangles.size()  == 24);
    REQUIRE(m.tetrahedra.size() == 17);
}

TEST_CASE("3D primitives live at w=0 with no tetrahedra", "[geometry][3d]") {
    for (auto t : {PrimitiveType::Cube3D, PrimitiveType::Tetrahedron3D,
                   PrimitiveType::Octahedron3D, PrimitiveType::Icosahedron3D,
                   PrimitiveType::Sphere3D, PrimitiveType::Cylinder3D,
                   PrimitiveType::Cone3D, PrimitiveType::Torus3D,
                   PrimitiveType::Pyramid3D, PrimitiveType::Prism3D})
    {
        const Mesh4D m = buildPrimitive(t);
        REQUIRE(!m.vertices.empty());
        REQUIRE(!m.triangles.empty());
        for (const auto& v : m.vertices) REQUIRE(v.w == 0.f);
        REQUIRE(m.tetrahedra.empty());
    }
}

TEST_CASE("2D primitives live at z=0 and w=0", "[geometry][2d]") {
    for (auto t : {PrimitiveType::Triangle2D, PrimitiveType::Square2D,
                   PrimitiveType::Pentagon2D, PrimitiveType::Hexagon2D,
                   PrimitiveType::Star2D, PrimitiveType::Circle2D})
    {
        const Mesh4D m = buildPrimitive(t);
        REQUIRE(!m.vertices.empty());
        REQUIRE(!m.edges.empty());
        REQUIRE(!m.triangles.empty());
        for (const auto& v : m.vertices) {
            REQUIRE(v.z == 0.f);
            REQUIRE(v.w == 0.f);
        }
        REQUIRE(m.tetrahedra.empty());
    }
}

TEST_CASE("Square has 4 vertices, 4 edges, 2 triangles", "[geometry][2d]") {
    const Mesh4D m = buildPrimitive(PrimitiveType::Square2D);
    REQUIRE(m.vertices.size()  == 4);
    REQUIRE(m.edges.size()     == 4);
    REQUIRE(m.triangles.size() == 2);
}

TEST_CASE("Triangle has 3 vertices, 3 edges, 1 triangle", "[geometry][2d]") {
    const Mesh4D m = buildPrimitive(PrimitiveType::Triangle2D);
    REQUIRE(m.vertices.size()  == 3);
    REQUIRE(m.edges.size()     == 3);
    REQUIRE(m.triangles.size() == 1);
}

TEST_CASE("Icosahedron has 12/30/20", "[geometry][3d]") {
    const Mesh4D m = buildPrimitive(PrimitiveType::Icosahedron3D);
    REQUIRE(m.vertices.size()  == 12);
    REQUIRE(m.edges.size()     == 30);
    REQUIRE(m.triangles.size() == 20);
}

TEST_CASE("primitiveIs2D / Is3D / Is4D classify correctly", "[geometry]") {
    REQUIRE(primitiveIs2D(PrimitiveType::Square2D));
    REQUIRE(primitiveIs3D(PrimitiveType::Cube3D));
    REQUIRE(primitiveIs4D(PrimitiveType::Tesseract));
    REQUIRE(primitiveIs4D(PrimitiveType::Icositetrachoron));
    REQUIRE(!primitiveIs4D(PrimitiveType::Cube3D));
    REQUIRE(!primitiveIs2D(PrimitiveType::Sphere3D));
    REQUIRE(!primitiveIs3D(PrimitiveType::Tesseract));
}
