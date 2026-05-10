#pragma once

#include "math/Mat5.h"
#include "math/Rotor4.h"
#include "math/Vec4.h"

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

enum class SliceAxis : int { X = 0, Y = 1, Z = 2, W = 3 };

// Whether to apply foreshortening (Perspective) or render with parallel projection
// in both 4D->3D and 3D->screen stages. Iso = isometric / orthographic.
enum class ProjectionStyle { Perspective, Isometric };

struct GridSettings {
    bool  showXY = false;
    bool  showXZ = true;   // ground plane in 3D conventions
    bool  showYZ = false;
    bool  showXW = false;
    bool  showYW = false;
    bool  showZW = false;
    float opacity = 0.35f;
};

struct Camera4D {
    math::Vec4   position4{};
    math::Rotor4 rotation4{};

    ViewMode     mode  = ViewMode::Projection;
    DisplayStyle style = DisplayStyle::DepthWireframe;

    // Slice mode: axis perpendicular to the cutting hyperplane + offset along that axis.
    SliceAxis sliceAxis = SliceAxis::W;
    float     sliceVal  = 0.f;

    ProjectionStyle projectionStyle = ProjectionStyle::Perspective;
    float           focal4  = 4.f;
    float           wOffset = 3.f;

    GridSettings grid{};

    float pivotX = 0.f, pivotY = 0.f, pivotZ = 0.f;
    float yaw      = 0.6f;
    float pitch    = 0.4f;
    float distance = 5.f;
    float fovYDeg  = 60.f;
    float zNear    = 0.05f;
    float zFar     = 200.f;
};

} // namespace hopf::render
