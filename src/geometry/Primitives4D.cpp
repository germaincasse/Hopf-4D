#include "PrimitivesInternal.h"

#include <array>
#include <cmath>
#include <cstdint>

namespace hopf::geometry::detail {

namespace {

// Sommerville split of a unit cube whose vertices are 3-bit binary codes (bit k -> axis k).
// The four "alternating" corners {1,2,4,7} share the inscribed central tet.
constexpr int kCubeTetSplit[5][4] = {
    {0, 1, 2, 4},
    {3, 1, 2, 7},
    {5, 1, 4, 7},
    {6, 2, 4, 7},
    {1, 2, 4, 7},
};

std::array<uint32_t, 8> cubeCellIndices(int fixedAxis, int fixedBit) {
    int freeAxes[3];
    int n = 0;
    for (int a = 0; a < 4; ++a) if (a != fixedAxis) freeAxes[n++] = a;

    std::array<uint32_t, 8> out{};
    for (int local = 0; local < 8; ++local) {
        int g = 0;
        if (fixedBit) g |= (1 << fixedAxis);
        for (int k = 0; k < 3; ++k) {
            if (local & (1 << k)) g |= (1 << freeAxes[k]);
        }
        out[local] = static_cast<uint32_t>(g);
    }
    return out;
}

} // namespace

Mesh4D buildTesseract(float size) {
    Mesh4D m;
    m.name = "Tesseract";
    const float h = size * 0.5f;

    m.vertices.reserve(16);
    for (int i = 0; i < 16; ++i) {
        m.vertices.push_back({
            (i & 1) ? h : -h,
            (i & 2) ? h : -h,
            (i & 4) ? h : -h,
            (i & 8) ? h : -h,
        });
    }
    for (uint32_t i = 0; i < 16; ++i) {
        for (int b = 0; b < 4; ++b) {
            const uint32_t j = i ^ (1u << b);
            if (j > i) m.edges.push_back({i, j});
        }
    }

    constexpr int axisPairs[6][2] = {{0,1},{0,2},{0,3},{1,2},{1,3},{2,3}};
    for (auto& pair : axisPairs) {
        const int a = pair[0];
        const int b = pair[1];
        int other[2];
        int oi = 0;
        for (int x = 0; x < 4; ++x) if (x != a && x != b) other[oi++] = x;

        for (int signMask = 0; signMask < 4; ++signMask) {
            const int s0 = (signMask >> 0) & 1;
            const int s1 = (signMask >> 1) & 1;
            uint32_t v[4];
            for (int corner = 0; corner < 4; ++corner) {
                int idx = 0;
                if ((corner >> 0) & 1) idx |= (1 << a);
                if ((corner >> 1) & 1) idx |= (1 << b);
                if (s0)                idx |= (1 << other[0]);
                if (s1)                idx |= (1 << other[1]);
                v[corner] = static_cast<uint32_t>(idx);
            }
            m.triangles.push_back({v[0], v[1], v[3]});
            m.triangles.push_back({v[0], v[3], v[2]});
        }
    }

    m.tetrahedra.reserve(8 * 5);
    for (int axis = 0; axis < 4; ++axis) {
        for (int side = 0; side < 2; ++side) {
            const auto cube = cubeCellIndices(axis, side);
            for (const auto& tet : kCubeTetSplit) {
                m.tetrahedra.push_back({cube[tet[0]], cube[tet[1]], cube[tet[2]], cube[tet[3]]});
            }
        }
    }
    return m;
}

Mesh4D buildPentachoron(float size) {
    Mesh4D m;
    m.name = "Pentachoron";

    // Regular 4-simplex coordinates rescaled so the base edge length equals `size`.
    const float invSqrt5 = 1.f / std::sqrt(5.f);
    const float k        = size / std::sqrt(8.f);

    m.vertices = {
        { k *  1.f, k *  1.f, k *  1.f, k * -invSqrt5},
        { k *  1.f, k * -1.f, k * -1.f, k * -invSqrt5},
        { k * -1.f, k *  1.f, k * -1.f, k * -invSqrt5},
        { k * -1.f, k * -1.f, k *  1.f, k * -invSqrt5},
        { 0.f,      0.f,      0.f,      k * 4.f * invSqrt5},
    };

    for (uint32_t i = 0; i < 5; ++i)
        for (uint32_t j = i + 1; j < 5; ++j)
            m.edges.push_back({i, j});

    for (uint32_t i = 0; i < 5; ++i)
        for (uint32_t j = i + 1; j < 5; ++j)
            for (uint32_t l = j + 1; l < 5; ++l)
                m.triangles.push_back({i, j, l});

    for (uint32_t skip = 0; skip < 5; ++skip) {
        std::array<uint32_t, 4> tet{};
        int n = 0;
        for (uint32_t i = 0; i < 5; ++i) if (i != skip) tet[n++] = i;
        m.tetrahedra.push_back(tet);
    }
    return m;
}

Mesh4D buildHexadecachoron(float size) {
    Mesh4D m;
    m.name = "Hexadecachoron";
    const float h = size * 0.5f;

    m.vertices = {
        { h, 0, 0, 0}, {-h, 0, 0, 0},
        { 0, h, 0, 0}, { 0,-h, 0, 0},
        { 0, 0, h, 0}, { 0, 0,-h, 0},
        { 0, 0, 0, h}, { 0, 0, 0,-h},
    };
    for (uint32_t i = 0; i < 8; ++i)
        for (uint32_t j = i + 1; j < 8; ++j)
            if (i / 2 != j / 2) m.edges.push_back({i, j});

    constexpr int axisTriples[4][3] = {{0,1,2},{0,1,3},{0,2,3},{1,2,3}};
    for (auto& tri : axisTriples) {
        for (int signMask = 0; signMask < 8; ++signMask) {
            m.triangles.push_back({
                static_cast<uint32_t>(tri[0] * 2 + ((signMask >> 0) & 1)),
                static_cast<uint32_t>(tri[1] * 2 + ((signMask >> 1) & 1)),
                static_cast<uint32_t>(tri[2] * 2 + ((signMask >> 2) & 1)),
            });
        }
    }
    for (int mask = 0; mask < 16; ++mask) {
        m.tetrahedra.push_back({
            static_cast<uint32_t>(0 + ((mask >> 0) & 1)),
            static_cast<uint32_t>(2 + ((mask >> 1) & 1)),
            static_cast<uint32_t>(4 + ((mask >> 2) & 1)),
            static_cast<uint32_t>(6 + ((mask >> 3) & 1)),
        });
    }
    return m;
}

Mesh4D buildIcositetrachoron(float size) {
    Mesh4D m;
    m.name = "Icositetrachoron";

    // 8 axis vertices at distance h from origin + 16 corner vertices at distance h too.
    // All 24 lie on the unit hypersphere of radius h; the regular 24-cell edge length is h.
    const float h = size * 0.5f;
    const float q = h * 0.5f;

    for (int axis = 0; axis < 4; ++axis) {
        for (int s = 0; s < 2; ++s) {
            math::Vec4 v{};
            v[axis] = (s == 0) ? h : -h;
            m.vertices.push_back(v);
        }
    }
    for (int mask = 0; mask < 16; ++mask) {
        m.vertices.push_back({
            (mask & 1) ? q : -q,
            (mask & 2) ? q : -q,
            (mask & 4) ? q : -q,
            (mask & 8) ? q : -q,
        });
    }

    const float edgeLen = h;
    const float eps     = 1e-3f * edgeLen;
    bool adj[24][24] = {};
    for (uint32_t i = 0; i < 24; ++i) {
        for (uint32_t j = i + 1; j < 24; ++j) {
            const auto& a = m.vertices[i];
            const auto& b = m.vertices[j];
            const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z, dw = a.w - b.w;
            const float d  = std::sqrt(dx*dx + dy*dy + dz*dz + dw*dw);
            if (std::abs(d - edgeLen) < eps) {
                m.edges.push_back({i, j});
                adj[i][j] = adj[j][i] = true;
            }
        }
    }

    for (uint32_t i = 0; i < 24; ++i)
        for (uint32_t j = i + 1; j < 24; ++j)
            if (adj[i][j])
                for (uint32_t k = j + 1; k < 24; ++k)
                    if (adj[i][k] && adj[j][k])
                        m.triangles.push_back({i, j, k});

    // Fan tetrahedrization from the centroid (origin). Less efficient than per-cell splitting
    // but doesn't require enumerating the 24 octahedral cells of the polytope.
    const uint32_t center = uint32_t(m.vertices.size());
    m.vertices.push_back({0.f, 0.f, 0.f, 0.f});
    for (const auto& tri : m.triangles) {
        m.tetrahedra.push_back({tri[0], tri[1], tri[2], center});
    }
    return m;
}

Mesh4D buildTetrahedralPrism(float size) {
    Mesh4D m;
    m.name = "Tetrahedral prism";
    const float s = size * 0.5f;
    const float w = size * 0.5f;

    m.vertices = {
        { s,  s,  s, -w}, { s, -s, -s, -w}, {-s,  s, -s, -w}, {-s, -s,  s, -w},
        { s,  s,  s,  w}, { s, -s, -s,  w}, {-s,  s, -s,  w}, {-s, -s,  s,  w},
    };
    m.edges = {
        {0,1},{0,2},{0,3},{1,2},{1,3},{2,3},
        {4,5},{4,6},{4,7},{5,6},{5,7},{6,7},
        {0,4},{1,5},{2,6},{3,7},
    };
    m.triangles = {
        {0,1,2},{0,1,3},{0,2,3},{1,2,3},
        {4,5,6},{4,5,7},{4,6,7},{5,6,7},
    };
    auto quad = [&](uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
        m.triangles.push_back({a, b, c});
        m.triangles.push_back({a, c, d});
    };
    quad(0, 1, 5, 4);
    quad(0, 2, 6, 4);
    quad(0, 3, 7, 4);
    quad(1, 2, 6, 5);
    quad(1, 3, 7, 5);
    quad(2, 3, 7, 6);

    m.tetrahedra = {{0, 1, 2, 3}, {4, 5, 6, 7}};
    auto prism = [&](uint32_t a, uint32_t b, uint32_t c,
                     uint32_t a2, uint32_t b2, uint32_t c2) {
        m.tetrahedra.push_back({a,  b,  c,  a2});
        m.tetrahedra.push_back({b,  c,  a2, b2});
        m.tetrahedra.push_back({c,  a2, b2, c2});
    };
    prism(0, 1, 2, 4, 5, 6);
    prism(0, 1, 3, 4, 5, 7);
    prism(0, 2, 3, 4, 6, 7);
    prism(1, 2, 3, 5, 6, 7);
    return m;
}

Mesh4D buildCubicalPyramid(float size) {
    Mesh4D m;
    m.name = "Cubical pyramid";
    const float h = size * 0.5f;

    for (int i = 0; i < 8; ++i) {
        m.vertices.push_back({
            (i & 1) ? h : -h,
            (i & 2) ? h : -h,
            (i & 4) ? h : -h,
            0.f,
        });
    }
    const uint32_t apex = uint32_t(m.vertices.size());
    m.vertices.push_back({0.f, 0.f, 0.f, h});

    for (uint32_t i = 0; i < 8; ++i) {
        for (int b = 0; b < 3; ++b) {
            const uint32_t j = i ^ (1u << b);
            if (j > i) m.edges.push_back({i, j});
        }
        m.edges.push_back({i, apex});
    }

    struct Face { uint32_t v[4]; };
    constexpr Face cubeFaces[6] = {
        {{0, 1, 3, 2}}, {{4, 6, 7, 5}}, {{0, 4, 5, 1}},
        {{2, 3, 7, 6}}, {{0, 2, 6, 4}}, {{1, 5, 7, 3}},
    };
    for (const auto& f : cubeFaces) {
        m.triangles.push_back({f.v[0], f.v[1], f.v[2]});
        m.triangles.push_back({f.v[0], f.v[2], f.v[3]});
    }
    for (uint32_t i = 0; i < 8; ++i) {
        for (int b = 0; b < 3; ++b) {
            const uint32_t j = i ^ (1u << b);
            if (j > i) m.triangles.push_back({i, j, apex});
        }
    }

    for (const auto& tet : kCubeTetSplit) {
        m.tetrahedra.push_back({
            uint32_t(tet[0]), uint32_t(tet[1]),
            uint32_t(tet[2]), uint32_t(tet[3]),
        });
    }
    for (const auto& f : cubeFaces) {
        m.tetrahedra.push_back({f.v[0], f.v[1], f.v[2], apex});
        m.tetrahedra.push_back({f.v[0], f.v[2], f.v[3], apex});
    }
    return m;
}

} // namespace hopf::geometry::detail
