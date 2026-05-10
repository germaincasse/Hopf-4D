#pragma once

#include <cmath>

namespace hopf::math {

struct Vec4 {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
    float w = 0.f;

    constexpr Vec4() = default;
    constexpr Vec4(float xx, float yy, float zz, float ww) : x(xx), y(yy), z(zz), w(ww) {}

    constexpr float operator[](int i) const { return (&x)[i]; }
    float& operator[](int i) { return (&x)[i]; }

    constexpr Vec4 operator+(const Vec4& v) const { return {x + v.x, y + v.y, z + v.z, w + v.w}; }
    constexpr Vec4 operator-(const Vec4& v) const { return {x - v.x, y - v.y, z - v.z, w - v.w}; }
    constexpr Vec4 operator-() const              { return {-x, -y, -z, -w}; }
    constexpr Vec4 operator*(float s) const       { return {x * s, y * s, z * s, w * s}; }
    constexpr Vec4 operator/(float s) const       { return {x / s, y / s, z / s, w / s}; }

    Vec4& operator+=(const Vec4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
    Vec4& operator-=(const Vec4& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
    Vec4& operator*=(float s)       { x *= s;   y *= s;   z *= s;   w *= s;   return *this; }
};

constexpr Vec4 operator*(float s, const Vec4& v) { return v * s; }

constexpr float dot(const Vec4& a, const Vec4& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline float length(const Vec4& v)        { return std::sqrt(dot(v, v)); }
inline float lengthSquared(const Vec4& v) { return dot(v, v); }

inline Vec4 normalize(const Vec4& v) {
    const float len = length(v);
    return len > 0.f ? v / len : Vec4{};
}

inline Vec4 lerp(const Vec4& a, const Vec4& b, float t) {
    return a + (b - a) * t;
}

} // namespace hopf::math
