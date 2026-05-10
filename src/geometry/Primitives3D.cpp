#include "PrimitivesInternal.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace hopf::geometry::detail {

namespace {

constexpr float kPi = 3.14159265358979323846f;

void addQuad(Mesh4D& m, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    m.triangles.push_back({a, b, c});
    m.triangles.push_back({a, c, d});
}

} // namespace

Mesh4D buildCube3D(float size) {
    Mesh4D m;
    m.name = "Cube";
    const float h = size * 0.5f;
    m.vertices.reserve(8);
    for (int i = 0; i < 8; ++i) {
        m.vertices.push_back({
            (i & 1) ? h : -h,
            (i & 2) ? h : -h,
            (i & 4) ? h : -h,
            0.f,
        });
    }
    for (uint32_t i = 0; i < 8; ++i) {
        for (int b = 0; b < 3; ++b) {
            const uint32_t j = i ^ (1u << b);
            if (j > i) m.edges.push_back({i, j});
        }
    }
    addQuad(m, 0, 1, 3, 2);
    addQuad(m, 4, 6, 7, 5);
    addQuad(m, 0, 4, 5, 1);
    addQuad(m, 2, 3, 7, 6);
    addQuad(m, 0, 2, 6, 4);
    addQuad(m, 1, 5, 7, 3);
    return m;
}

Mesh4D buildTetrahedron3D(float size) {
    Mesh4D m;
    m.name = "Tetrahedron";
    const float h = size * 0.5f;
    m.vertices = {
        { h,  h,  h, 0.f},
        { h, -h, -h, 0.f},
        {-h,  h, -h, 0.f},
        {-h, -h,  h, 0.f},
    };
    m.edges     = {{0,1},{0,2},{0,3},{1,2},{1,3},{2,3}};
    m.triangles = {{0,1,2},{0,1,3},{0,2,3},{1,2,3}};
    return m;
}

Mesh4D buildOctahedron3D(float size) {
    Mesh4D m;
    m.name = "Octahedron";
    const float h = size * 0.5f;
    m.vertices = {
        { h, 0, 0, 0}, {-h, 0, 0, 0},
        { 0, h, 0, 0}, { 0,-h, 0, 0},
        { 0, 0, h, 0}, { 0, 0,-h, 0},
    };
    for (uint32_t i = 0; i < 6; ++i)
        for (uint32_t j = i + 1; j < 6; ++j)
            if (i / 2 != j / 2) m.edges.push_back({i, j});

    for (int sx = 0; sx < 2; ++sx)
        for (int sy = 0; sy < 2; ++sy)
            for (int sz = 0; sz < 2; ++sz) {
                m.triangles.push_back({
                    static_cast<uint32_t>(0 + sx),
                    static_cast<uint32_t>(2 + sy),
                    static_cast<uint32_t>(4 + sz),
                });
            }
    return m;
}

Mesh4D buildIcosahedron3D(float size) {
    Mesh4D m;
    m.name = "Icosahedron";
    const float t = (1.f + std::sqrt(5.f)) * 0.5f;
    const float k = (size * 0.5f) / std::sqrt(1.f + t * t);

    const float T = t * k;
    m.vertices = {
        {-k,  T, 0, 0}, { k,  T, 0, 0}, {-k, -T, 0, 0}, { k, -T, 0, 0},
        { 0, -k,  T, 0}, { 0,  k,  T, 0}, { 0, -k, -T, 0}, { 0,  k, -T, 0},
        { T,  0, -k, 0}, { T,  0,  k, 0}, {-T,  0, -k, 0}, {-T,  0,  k, 0},
    };

    constexpr uint32_t faces[20][3] = {
        { 0, 11,  5}, { 0,  5,  1}, { 0,  1,  7}, { 0,  7, 10}, { 0, 10, 11},
        { 1,  5,  9}, { 5, 11,  4}, {11, 10,  2}, {10,  7,  6}, { 7,  1,  8},
        { 3,  9,  4}, { 3,  4,  2}, { 3,  2,  6}, { 3,  6,  8}, { 3,  8,  9},
        { 4,  9,  5}, { 2,  4, 11}, { 6,  2, 10}, { 8,  6,  7}, { 9,  8,  1},
    };
    m.triangles.reserve(20);
    for (const auto& f : faces) m.triangles.push_back({f[0], f[1], f[2]});

    auto addEdge = [&](uint32_t a, uint32_t b) {
        if (a > b) std::swap(a, b);
        for (const auto& e : m.edges) if (e[0] == a && e[1] == b) return;
        m.edges.push_back({a, b});
    };
    for (const auto& f : faces) {
        addEdge(f[0], f[1]);
        addEdge(f[1], f[2]);
        addEdge(f[0], f[2]);
    }
    return m;
}

Mesh4D buildSphere3D(float diameter) {
    constexpr int latRings = 16;
    constexpr int lonSegs  = 24;

    Mesh4D m;
    m.name = "Sphere";
    const float r = diameter * 0.5f;

    for (int i = 0; i <= latRings; ++i) {
        const float theta = float(i) / float(latRings) * kPi;
        const float sinT  = std::sin(theta);
        const float cosT  = std::cos(theta);
        for (int j = 0; j <= lonSegs; ++j) {
            const float phi = float(j) / float(lonSegs) * 2.f * kPi;
            m.vertices.push_back({
                r * sinT * std::cos(phi),
                r * cosT,
                r * sinT * std::sin(phi),
                0.f,
            });
        }
    }

    auto idx = [&](int i, int j) {
        return static_cast<uint32_t>(i * (lonSegs + 1) + j);
    };

    for (int i = 0; i < latRings; ++i) {
        for (int j = 0; j < lonSegs; ++j) {
            const uint32_t a = idx(i,     j);
            const uint32_t b = idx(i + 1, j);
            const uint32_t c = idx(i + 1, j + 1);
            const uint32_t d = idx(i,     j + 1);
            m.triangles.push_back({a, b, c});
            m.triangles.push_back({a, c, d});
            m.edges.push_back({a, d});
            if (i + 1 < latRings) m.edges.push_back({a, b});
        }
    }
    return m;
}

Mesh4D buildCylinder3D(float size) {
    constexpr int N = 24;
    Mesh4D m;
    m.name = "Cylinder";
    const float r = size * 0.5f;
    const float h = size * 0.5f;

    for (int i = 0; i < N; ++i) {
        const float a = 2.f * kPi * float(i) / float(N);
        m.vertices.push_back({ r * std::cos(a), -h, r * std::sin(a), 0.f });
    }
    for (int i = 0; i < N; ++i) {
        const float a = 2.f * kPi * float(i) / float(N);
        m.vertices.push_back({ r * std::cos(a),  h, r * std::sin(a), 0.f });
    }
    const uint32_t bottomCenter = uint32_t(m.vertices.size());
    m.vertices.push_back({ 0.f, -h, 0.f, 0.f });
    const uint32_t topCenter = uint32_t(m.vertices.size());
    m.vertices.push_back({ 0.f,  h, 0.f, 0.f });

    for (uint32_t i = 0; i < N; ++i) {
        const uint32_t b0 = i;
        const uint32_t b1 = (i + 1) % N;
        const uint32_t t0 = N + i;
        const uint32_t t1 = N + (i + 1) % N;
        m.triangles.push_back({b0, b1, t1});
        m.triangles.push_back({b0, t1, t0});
        m.triangles.push_back({bottomCenter, b1, b0});
        m.triangles.push_back({topCenter,    t0, t1});
        m.edges.push_back({b0, b1});
        m.edges.push_back({t0, t1});
        m.edges.push_back({b0, t0});
    }
    return m;
}

Mesh4D buildCone3D(float size) {
    constexpr int N = 24;
    Mesh4D m;
    m.name = "Cone";
    const float r = size * 0.5f;
    const float h = size * 0.5f;

    for (int i = 0; i < N; ++i) {
        const float a = 2.f * kPi * float(i) / float(N);
        m.vertices.push_back({ r * std::cos(a), -h, r * std::sin(a), 0.f });
    }
    const uint32_t apex = uint32_t(m.vertices.size());
    m.vertices.push_back({ 0.f,  h, 0.f, 0.f });
    const uint32_t bottomCenter = uint32_t(m.vertices.size());
    m.vertices.push_back({ 0.f, -h, 0.f, 0.f });

    for (uint32_t i = 0; i < N; ++i) {
        const uint32_t b0 = i;
        const uint32_t b1 = (i + 1) % N;
        m.triangles.push_back({b0, b1, apex});
        m.triangles.push_back({bottomCenter, b1, b0});
        m.edges.push_back({b0, b1});
        m.edges.push_back({b0, apex});
    }
    return m;
}

Mesh4D buildTorus3D(float size) {
    constexpr int M = 24;
    constexpr int N = 12;
    Mesh4D m;
    m.name = "Torus";
    const float R = size * 0.35f;
    const float r = size * 0.15f;

    auto idx = [&](int i, int j) {
        return uint32_t(i * N + j);
    };

    for (int i = 0; i < M; ++i) {
        const float u = 2.f * kPi * float(i) / float(M);
        for (int j = 0; j < N; ++j) {
            const float v = 2.f * kPi * float(j) / float(N);
            const float ringR = R + r * std::cos(v);
            m.vertices.push_back({
                ringR * std::cos(u),
                r * std::sin(v),
                ringR * std::sin(u),
                0.f,
            });
        }
    }
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            const uint32_t a = idx(i, j);
            const uint32_t b = idx((i + 1) % M, j);
            const uint32_t c = idx((i + 1) % M, (j + 1) % N);
            const uint32_t d = idx(i, (j + 1) % N);
            m.triangles.push_back({a, b, c});
            m.triangles.push_back({a, c, d});
            m.edges.push_back({a, b});
            m.edges.push_back({a, d});
        }
    }
    return m;
}

Mesh4D buildPyramid3D(float size) {
    Mesh4D m;
    m.name = "Pyramid";
    const float h = size * 0.5f;
    m.vertices = {
        {-h, -h, -h, 0.f},
        { h, -h, -h, 0.f},
        { h, -h,  h, 0.f},
        {-h, -h,  h, 0.f},
        { 0.f,  h, 0.f, 0.f},
    };
    m.edges = {
        {0,1},{1,2},{2,3},{3,0},
        {0,4},{1,4},{2,4},{3,4},
    };
    m.triangles = {
        {0, 1, 2}, {0, 2, 3},
        {0, 1, 4}, {1, 2, 4}, {2, 3, 4}, {3, 0, 4},
    };
    return m;
}

Mesh4D buildPrism3D(float size) {
    Mesh4D m;
    m.name = "Prism";
    const float h = size * 0.5f;
    const float r = size * 0.5f;

    for (int i = 0; i < 3; ++i) {
        const float a = kPi * 0.5f + 2.f * kPi * float(i) / 3.f;
        m.vertices.push_back({ r * std::cos(a), -h, r * std::sin(a), 0.f });
    }
    for (int i = 0; i < 3; ++i) {
        const float a = kPi * 0.5f + 2.f * kPi * float(i) / 3.f;
        m.vertices.push_back({ r * std::cos(a),  h, r * std::sin(a), 0.f });
    }
    m.edges = {
        {0,1},{1,2},{2,0},
        {3,4},{4,5},{5,3},
        {0,3},{1,4},{2,5},
    };
    m.triangles = {
        {0, 1, 2},
        {3, 5, 4},
    };
    addQuad(m, 0, 1, 4, 3);
    addQuad(m, 1, 2, 5, 4);
    addQuad(m, 2, 0, 3, 5);
    return m;
}

} // namespace hopf::geometry::detail
