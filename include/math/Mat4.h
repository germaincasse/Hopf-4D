#pragma once

#include <array>
#include <cmath>

namespace hopf::math {

// Column-major 4x4 matrix, GL-friendly. Used for the 3D view/projection stage that
// follows the 4D->3D step.
using Mat4 = std::array<float, 16>;

inline Mat4 mat4Identity() {
    Mat4 m{};
    m[0] = m[5] = m[10] = m[15] = 1.f;
    return m;
}

inline Mat4 mat4Mul(const Mat4& a, const Mat4& b) {
    Mat4 r{};
    for (int c = 0; c < 4; ++c) {
        for (int row = 0; row < 4; ++row) {
            float s = 0.f;
            for (int k = 0; k < 4; ++k) s += a[k * 4 + row] * b[c * 4 + k];
            r[c * 4 + row] = s;
        }
    }
    return r;
}

inline Mat4 perspective(float fovYRad, float aspect, float zn, float zf) {
    const float f = 1.f / std::tan(fovYRad * 0.5f);
    Mat4 m{};
    m[0]  = f / aspect;
    m[5]  = f;
    m[10] = (zf + zn) / (zn - zf);
    m[11] = -1.f;
    m[14] = (2.f * zf * zn) / (zn - zf);
    return m;
}

inline Mat4 ortho(float left, float right, float bottom, float top, float zn, float zf) {
    Mat4 m{};
    m[0]  =  2.f / (right - left);
    m[5]  =  2.f / (top - bottom);
    m[10] = -2.f / (zf - zn);
    m[12] = -(right + left)   / (right - left);
    m[13] = -(top   + bottom) / (top   - bottom);
    m[14] = -(zf    + zn)     / (zf    - zn);
    m[15] = 1.f;
    return m;
}

inline Mat4 lookAt(float ex, float ey, float ez,
                   float cx, float cy, float cz,
                   float ux, float uy, float uz)
{
    float fx = cx - ex, fy = cy - ey, fz = cz - ez;
    const float fl = std::sqrt(fx*fx + fy*fy + fz*fz);
    if (fl > 0.f) { fx /= fl; fy /= fl; fz /= fl; }

    float sx = fy * uz - fz * uy;
    float sy = fz * ux - fx * uz;
    float sz = fx * uy - fy * ux;
    const float sl = std::sqrt(sx*sx + sy*sy + sz*sz);
    if (sl > 0.f) { sx /= sl; sy /= sl; sz /= sl; }

    const float u2x = sy * fz - sz * fy;
    const float u2y = sz * fx - sx * fz;
    const float u2z = sx * fy - sy * fx;

    Mat4 m{};
    m[0] = sx;  m[4] = sy;  m[8]  = sz;  m[12] = -(sx*ex + sy*ey + sz*ez);
    m[1] = u2x; m[5] = u2y; m[9]  = u2z; m[13] = -(u2x*ex + u2y*ey + u2z*ez);
    m[2] = -fx; m[6] = -fy; m[10] = -fz; m[14] =   fx*ex + fy*ey + fz*ez;
    m[3] = 0;   m[7] = 0;   m[11] = 0;   m[15] = 1.f;
    return m;
}

} // namespace hopf::math
