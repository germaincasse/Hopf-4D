#pragma once

#include "math/Vec4.h"

#include <array>
#include <cmath>

namespace hopf::math {

// 5x5 matrix used for homogeneous 4D affine transforms. Column-major to match GL.
// A Vec4 v is treated as (v.x, v.y, v.z, v.w, 1) when transformed.
struct Mat5 {
    std::array<std::array<float, 5>, 5> m{}; // m[col][row]

    static constexpr Mat5 identity() {
        Mat5 r{};
        for (int i = 0; i < 5; ++i) r.m[i][i] = 1.f;
        return r;
    }

    static constexpr Mat5 zero() { return Mat5{}; }

    constexpr float& at(int col, int row)       { return m[col][row]; }
    constexpr float  at(int col, int row) const { return m[col][row]; }

    Mat5 operator*(const Mat5& rhs) const {
        Mat5 r = Mat5::zero();
        for (int c = 0; c < 5; ++c) {
            for (int rr = 0; rr < 5; ++rr) {
                float s = 0.f;
                for (int k = 0; k < 5; ++k) s += m[k][rr] * rhs.m[c][k];
                r.m[c][rr] = s;
            }
        }
        return r;
    }

    Vec4 transformPoint(const Vec4& v) const {
        const float h[5] = {v.x, v.y, v.z, v.w, 1.f};
        float out[5] = {0, 0, 0, 0, 0};
        for (int rr = 0; rr < 5; ++rr) {
            for (int c = 0; c < 5; ++c) out[rr] += m[c][rr] * h[c];
        }
        const float inv = out[4] != 0.f ? 1.f / out[4] : 1.f;
        return {out[0] * inv, out[1] * inv, out[2] * inv, out[3] * inv};
    }

    Vec4 transformDirection(const Vec4& v) const {
        const float h[5] = {v.x, v.y, v.z, v.w, 0.f};
        float out[4] = {0, 0, 0, 0};
        for (int rr = 0; rr < 4; ++rr) {
            for (int c = 0; c < 5; ++c) out[rr] += m[c][rr] * h[c];
        }
        return {out[0], out[1], out[2], out[3]};
    }
};

inline Mat5 translate4(const Vec4& t) {
    Mat5 r = Mat5::identity();
    r.at(4, 0) = t.x;
    r.at(4, 1) = t.y;
    r.at(4, 2) = t.z;
    r.at(4, 3) = t.w;
    return r;
}

inline Mat5 scale4(const Vec4& s) {
    Mat5 r = Mat5::identity();
    r.at(0, 0) = s.x;
    r.at(1, 1) = s.y;
    r.at(2, 2) = s.z;
    r.at(3, 3) = s.w;
    return r;
}

inline Mat5 scale4(float s) { return scale4({s, s, s, s}); }

// Rotation in the coordinate plane spanned by axes a and b (0=x, 1=y, 2=z, 3=w).
// 4D has six such planes: xy, xz, xw, yz, yw, zw.
inline Mat5 rotatePlane(int a, int b, float angleRadians) {
    Mat5 r = Mat5::identity();
    const float c = std::cos(angleRadians);
    const float s = std::sin(angleRadians);
    r.at(a, a) = c;
    r.at(b, b) = c;
    r.at(b, a) = -s;
    r.at(a, b) =  s;
    return r;
}

inline Mat5 rotateXY(float a) { return rotatePlane(0, 1, a); }
inline Mat5 rotateXZ(float a) { return rotatePlane(0, 2, a); }
inline Mat5 rotateXW(float a) { return rotatePlane(0, 3, a); }
inline Mat5 rotateYZ(float a) { return rotatePlane(1, 2, a); }
inline Mat5 rotateYW(float a) { return rotatePlane(1, 3, a); }
inline Mat5 rotateZW(float a) { return rotatePlane(2, 3, a); }

} // namespace hopf::math
