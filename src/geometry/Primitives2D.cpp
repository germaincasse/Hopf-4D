#include "PrimitivesInternal.h"

#include <cmath>
#include <cstdint>

namespace hopf::geometry::detail {

namespace {

constexpr float kPi = 3.14159265358979323846f;

Mesh4D regularPolygon(const char* name, int n, float diameter) {
    Mesh4D m;
    m.name = name;
    const float r = diameter * 0.5f;
    m.vertices.reserve(n);
    for (int i = 0; i < n; ++i) {
        const float a = 2.f * kPi * float(i) / float(n);
        m.vertices.push_back({ r * std::cos(a), r * std::sin(a), 0.f, 0.f });
    }
    for (uint32_t i = 0; i < uint32_t(n); ++i) {
        m.edges.push_back({ i, uint32_t((i + 1) % n) });
    }
    for (uint32_t i = 1; i + 1 < uint32_t(n); ++i) {
        m.triangles.push_back({ 0, i, uint32_t(i + 1) });
    }
    return m;
}

} // namespace

Mesh4D buildTriangle2D(float size) { return regularPolygon("Triangle", 3, size); }
Mesh4D buildPentagon2D(float size) { return regularPolygon("Pentagon", 5, size); }
Mesh4D buildHexagon2D(float size)  { return regularPolygon("Hexagon",  6, size); }
Mesh4D buildCircle2D(float size)   { return regularPolygon("Circle",  32, size); }

Mesh4D buildSquare2D(float size) {
    Mesh4D m;
    m.name = "Square";
    const float h = size * 0.5f;
    m.vertices = {
        { h,  h, 0, 0},
        {-h,  h, 0, 0},
        {-h, -h, 0, 0},
        { h, -h, 0, 0},
    };
    m.edges     = {{0,1},{1,2},{2,3},{3,0}};
    m.triangles = {{0,1,2},{0,2,3}};
    return m;
}

Mesh4D buildStar2D(float size) {
    Mesh4D m;
    m.name = "Star";
    constexpr int kPoints = 5;
    const float R = size * 0.5f;
    const float r = R * 0.4f;

    m.vertices.reserve(2 * kPoints + 1);
    for (int i = 0; i < 2 * kPoints; ++i) {
        const float radius = (i & 1) ? r : R;
        const float a = kPi * 0.5f + kPi * float(i) / float(kPoints);
        m.vertices.push_back({ radius * std::cos(a), radius * std::sin(a), 0.f, 0.f });
    }
    const uint32_t n = 2 * kPoints;
    for (uint32_t i = 0; i < n; ++i) {
        m.edges.push_back({ i, (i + 1) % n });
    }
    const uint32_t center = uint32_t(m.vertices.size());
    m.vertices.push_back({ 0, 0, 0, 0 });
    for (uint32_t i = 0; i < n; ++i) {
        m.triangles.push_back({ center, i, (i + 1) % n });
    }
    return m;
}

} // namespace hopf::geometry::detail
