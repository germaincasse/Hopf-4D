#pragma once

#include "hopf/math/Mat5.h"
#include "hopf/math/Rotor4.h"
#include "hopf/math/Vec4.h"

namespace hopf::render {

enum class ViewMode {
    Slice,
    Projection
};

enum class DisplayStyle {
    SimpleWireframe,
    DepthWireframe,
    SolidUnlit,
    SolidLit,
};

struct Camera4D {
    math::Vec4   position4{};
    math::Rotor4 rotation4{};

    ViewMode     mode  = ViewMode::Projection;
    DisplayStyle style = DisplayStyle::DepthWireframe;

    float sliceW = 0.f;

    bool  perspective4 = true;
    float focal4  = 4.f;
    float wOffset = 3.f;

    float pivotX = 0.f, pivotY = 0.f, pivotZ = 0.f;
    float yaw      = 0.6f;
    float pitch    = 0.4f;
    float distance = 5.f;
    float fovYDeg  = 60.f;
    float zNear    = 0.05f;
    float zFar     = 200.f;
};

} // namespace hopf::render
