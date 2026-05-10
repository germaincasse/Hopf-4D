#pragma once

#include "math/Mat5.h"
#include "math/Rotor4.h"
#include "math/Vec4.h"

namespace hopf::scene {

struct Transform4D {
    math::Vec4   position{0.f, 0.f, 0.f, 0.f};
    math::Rotor4 rotation{};
    math::Vec4   scale{1.f, 1.f, 1.f, 1.f};

    math::Mat5 toMatrix() const {
        return math::translate4(position) * rotation.toMatrix() * math::scale4(scale);
    }
};

} // namespace hopf::scene
