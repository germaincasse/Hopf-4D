#pragma once

#include "geometry/Mesh4D.h"
#include "math/Bivector4.h"
#include "scene/Transform4D.h"

#include <cstdint>
#include <optional>
#include <string>

namespace hopf::scene {

using EntityId = uint64_t;

// Per-plane angular velocities (rad/s). Identity = static.
struct AutoRotate {
    float xy = 0.f;
    float xz = 0.f;
    float xw = 0.f;
    float yz = 0.f;
    float yw = 0.f;
    float zw = 0.f;
};

// The light's direction is derived from the owning entity's transform.rotation, applied
// to the canonical "down" vector (0, -1, 0, 0). Rotate the entity (via the Inspector or
// auto-rotate) to change where the light shines from.
struct DirectionalLight {
    float intensity = 1.f;
};

struct CameraBackground {
    float r = 0.10f;
    float g = 0.11f;
    float b = 0.13f;
};

struct Camera2DComponent {
    float orthoSize = 5.f;
    float zNear     = -100.f;
    float zFar      =  100.f;
    CameraBackground bg{};
};

struct Camera3DComponent {
    bool  perspective = true;
    float fovYDeg     = 60.f;
    float orthoHalfH  = 5.f;
    float zNear       = 0.05f;
    float zFar        = 200.f;
    CameraBackground bg{};
};

struct Camera4DComponent {
    enum class Mode : int { Projection = 0, Slice = 1 };
    Mode  mode         = Mode::Projection;
    bool  perspective4 = true;
    float focal4       = 4.f;
    float wOffset      = 3.f;
    int   sliceAxis    = 3; // 0=X .. 3=W
    float sliceVal     = 0.f;
    bool  isMain       = false; // first one with this flag drives the Game view
    CameraBackground bg{};
};

struct MeshColliderComponent {
    int  dimension = 4; // 2, 3, or 4 — auto-set from the entity's mesh on add
    bool isTrigger = false;
};

struct RigidbodyComponent {
    int             dimension       = 4;
    float           mass            = 1.f;
    bool            kinematic       = false;
    bool            useGravity      = true;
    math::Vec4      velocity        = {};
    math::Bivector4 angularVelocity = {};
};

inline int meshDimension(const geometry::Mesh4D& m) {
    bool anyW = false;
    bool anyZ = false;
    for (const auto& v : m.vertices) {
        if (v.w != 0.f) anyW = true;
        if (v.z != 0.f) anyZ = true;
    }
    return anyW ? 4 : (anyZ ? 3 : 2);
}

struct Entity {
    EntityId                          id = 0;
    std::string                       name;
    Transform4D                       transform;
    const geometry::Mesh4D*           mesh = nullptr; // owned by Scene
    bool                              visible    = true;
    AutoRotate                        autoRotate{};
    std::optional<DirectionalLight>      light;
    std::optional<Camera2DComponent>     camera2D;
    std::optional<Camera3DComponent>     camera3D;
    std::optional<Camera4DComponent>     camera4D;
    std::optional<MeshColliderComponent> collider;
    std::optional<RigidbodyComponent>    rigidbody;
};

} // namespace hopf::scene
