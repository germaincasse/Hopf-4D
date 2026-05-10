#pragma once

#include "math/Mat5.h"

namespace hopf::math {

// 4D rotation as six per-plane angles (xy, xz, xw, yz, yw, zw), composed in fixed order.
// Editor-friendly but not commutative; for animation pipelines we'd switch to bivector rotors.
struct Rotor4 {
    float xy = 0.f;
    float xz = 0.f;
    float xw = 0.f;
    float yz = 0.f;
    float yw = 0.f;
    float zw = 0.f;

    Mat5 toMatrix() const {
        return rotateXY(xy) * rotateXZ(xz) * rotateXW(xw)
             * rotateYZ(yz) * rotateYW(yw) * rotateZW(zw);
    }

    Mat5 toInverseMatrix() const {
        return rotateZW(-zw) * rotateYW(-yw) * rotateYZ(-yz)
             * rotateXW(-xw) * rotateXZ(-xz) * rotateXY(-xy);
    }
};

} // namespace hopf::math
