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

// Distinguishes the 3D->screen projection from the 4D->3D projection so a scene
// can be viewed with screen perspective but a parallel 4D collapse (or vice
// versa). Three combinations are exposed:
//   Perspective : 3D persp + 4D persp
//   Hybrid      : 3D persp + 4D iso       (3D-only perspective)
//   Isometric   : 3D iso   + 4D iso
enum class ProjectionStyle { Perspective, Isometric, Hybrid };

inline bool perspective3D(ProjectionStyle s) { return s != ProjectionStyle::Isometric; }
inline bool perspective4D(ProjectionStyle s) { return s == ProjectionStyle::Perspective; }

struct GridSettings {
    bool  showXY = false;
    bool  showXZ = true;   // ground plane in 3D conventions
    bool  showYZ = false;
    bool  showXW = false;
    bool  showYW = false;
    bool  showZW = false;
    float opacity = 0.35f;
    int   cells   = 100;   // grid extent: cells/2 lines on either side of the origin
};

struct Camera4D {
    math::Vec4   position4{};
    math::Rotor4 rotation4{};

    ViewMode     mode  = ViewMode::Projection;
    DisplayStyle style = DisplayStyle::DepthWireframe;

    // Slice mode: arbitrary 4D normal of the cutting hyperplane + offset along it.
    // Default = pure +W axis (matches the old fixed sliceAxis=W).
    math::Vec4 sliceAxis{0.f, 0.f, 0.f, 1.f};
    float      sliceVal = 0.f;

    ProjectionStyle projectionStyle = ProjectionStyle::Hybrid;
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

    float bgR = 0.10f, bgG = 0.11f, bgB = 0.13f;

    // World-axes gizmo overlay (origin RGB + W). Editor viewports want it,
    // in-game cameras (Game view) typically don't.
    bool showWorldAxes = true;
};

} // namespace hopf::render
