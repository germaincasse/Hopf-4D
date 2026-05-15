#pragma once

#include "math/Vec4.h"

namespace hopf::math {

// Bivector in 4D: six components, one per coordinate plane. Represents either a
// rotation rate (rad/s per plane) or an angular impulse, depending on context.
struct Bivector4 {
    float xy = 0.f, xz = 0.f, xw = 0.f;
    float yz = 0.f, yw = 0.f, zw = 0.f;
};

// Exterior (wedge) product of two 4D vectors. Used to build angular impulses from
// a lever arm r and a linear impulse j: tau = wedge(r, j).
inline Bivector4 wedge(const Vec4& a, const Vec4& b) {
    return {
        a.x * b.y - a.y * b.x,
        a.x * b.z - a.z * b.x,
        a.x * b.w - a.w * b.x,
        a.y * b.z - a.z * b.y,
        a.y * b.w - a.w * b.y,
        a.z * b.w - a.w * b.z,
    };
}

// Linear velocity at point r induced by an angular velocity bivector w.
// In each plane (i, j), w_ij contributes v_i = -w_ij * r_j and v_j = +w_ij * r_i.
inline Vec4 velocityAt(const Bivector4& w, const Vec4& r) {
    return {
        -w.xy * r.y - w.xz * r.z - w.xw * r.w,
        +w.xy * r.x - w.yz * r.z - w.yw * r.w,
        +w.xz * r.x + w.yz * r.y - w.zw * r.w,
        +w.xw * r.x + w.yw * r.y + w.zw * r.z,
    };
}

} // namespace hopf::math
