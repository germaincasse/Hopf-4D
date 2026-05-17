#pragma once

#include "geometry/Mesh4D.h"
#include "math/Bivector4.h"
#include "scene/Transform4D.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace hopf::scripting { class Script; }

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
    float r = 1.f, g = 1.f, b = 1.f; // light tint, multiplied with the diffuse term
};

struct CameraBackground {
    float r = 0.10f;
    float g = 0.11f;
    float b = 0.13f;
};

// Mirrors render::DisplayStyle but lives in scene/ so components don't depend on
// render/. Mapped at render time in the Game view.
enum class CameraDisplayStyle : int { Wire = 0, Depth = 1, Unlit = 2, Lit = 3 };

struct Camera2DComponent {
    float orthoSize = 5.f;
    float zNear     = -100.f;
    float zFar      =  100.f;
    CameraDisplayStyle displayStyle = CameraDisplayStyle::Lit;
    CameraBackground bg{};
};

struct Camera3DComponent {
    bool  perspective = false; // default Iso
    float fovYDeg     = 60.f;
    float orthoHalfH  = 5.f;
    float zNear       = 0.05f;
    float zFar        = 200.f;
    CameraDisplayStyle displayStyle = CameraDisplayStyle::Lit;
    CameraBackground bg{};
};

// Mutually exclusive 4D camera projection presets. Distinguishes plain isometric
// from 3D-only perspective (Hybrid) from full 4D foreshortening.
enum class CameraProjection : int {
    Isometric   = 0, // 3D iso   + 4D iso
    Perspective3D = 1, // 3D persp + 4D iso (Hybrid)
    Perspective4D = 2, // 3D persp + 4D persp
};

struct Camera4DComponent {
    enum class Mode : int { Projection = 0, Slice = 1 };
    Mode       mode = Mode::Projection;
    CameraProjection projection = CameraProjection::Perspective3D;
    float      focal4   = 4.f;
    float      wOffset  = 3.f;
    // Slice mode: arbitrary 4D normal of the cutting hyperplane (need not be axis-aligned).
    math::Vec4 sliceAxis{0.f, 0.f, 0.f, 1.f};
    float      sliceVal = 0.f;
    bool       isMain   = false; // first one with this flag drives the Game view
    CameraDisplayStyle displayStyle = CameraDisplayStyle::Lit;
    CameraBackground bg{};
};

struct MeshColliderComponent {
    int  dimension = 4; // 2, 3, or 4 — auto-set from the entity's mesh on add
    bool isTrigger = false;
};

// Per-entity tint applied multiplicatively to the renderer's base color. Lets
// scripts colour individual meshes (e.g. minesweeper cells) without needing a
// material system. Multiplied with both Solid Unlit and Solid Lit output.
struct ColorTint {
    float r = 1.f, g = 1.f, b = 1.f;
};

struct RigidbodyComponent {
    int             dimension       = 4;
    float           mass            = 1.f;
    bool            kinematic       = false;
    bool            useGravity      = true;
    math::Vec4      velocity        = {};
    math::Bivector4 angularVelocity = {};
};

// Plays a clip when triggered. Spatial sources attenuate with distance from a listener;
// non-spatial sources play at constant volume regardless of position. Bus routes to a
// named mixer track defined in the project's audio settings.
struct AudioSourceComponent {
    std::string clipPath;
    float       volume      = 1.f;
    float       pitch       = 1.f;
    bool        loop        = false;
    bool        playOnStart = false;
    bool        spatial     = true;
    float       maxDistance = 25.f;
    std::string bus         = "master";
};

// Marks the entity as the audio listener (typically attached to a camera).
// The renderer/audio engine uses this entity's transform to compute attenuation.
struct AudioListenerComponent {
    bool active = true;
};

enum class UIAnchor : int {
    TopLeft = 0, TopCenter, TopRight,
    MiddleLeft,  Center,    MiddleRight,
    BottomLeft,  BottomCenter, BottomRight,
};

struct UIRectComponent {
    UIAnchor anchor = UIAnchor::Center;
    float    x = 0.f, y = 0.f;            // pixel offset from anchor
    float    width = 200.f, height = 80.f;
    float    r = 0.2f, g = 0.2f, b = 0.25f, a = 1.f;
};

struct UITextComponent {
    std::string text     = "Hello";
    float       fontSize = 24.f;
    float       r = 1.f, g = 1.f, b = 1.f, a = 1.f;
};

struct UIImageComponent {
    std::string imagePath;
    float       r = 1.f, g = 1.f, b = 1.f, a = 1.f; // tint
};

struct UIButtonComponent {
    float r = 0.25f, g = 0.40f, b = 0.65f, a = 1.f;
    float hoverR = 0.35f, hoverG = 0.55f, hoverB = 0.85f, hoverA = 1.f;
    std::string onClickScript; // script type name that should receive the callback
    std::string onClickMethod = "onClick"; // method on that script (looked up at runtime)
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

// A script bound to an entity. Both data fields persist; the runtime instance is
// re-created from the type name during snapshot/restore and entity duplication.
struct ScriptInstance {
    std::string                            typeName;
    std::unique_ptr<scripting::Script>     instance;
};

struct Entity {
    EntityId                                  id        = 0;
    EntityId                                  parent    = 0; // 0 = root entity
    std::string                               name;
    Transform4D                               transform;
    const geometry::Mesh4D*                   mesh      = nullptr; // owned by Scene
    // For meshes created via Scene::addPrimitive, holds the enum name so the scene
    // can round-trip through serialization. Empty for custom/imported meshes.
    std::string                               primitiveType;
    bool                                      visible   = true;
    AutoRotate                                autoRotate{};
    std::optional<DirectionalLight>           light;
    std::optional<Camera2DComponent>          camera2D;
    std::optional<Camera3DComponent>          camera3D;
    std::optional<Camera4DComponent>          camera4D;
    std::optional<MeshColliderComponent>      collider;
    std::optional<RigidbodyComponent>         rigidbody;
    std::optional<ColorTint>                  tint;
    std::optional<AudioSourceComponent>       audioSource;
    std::optional<AudioListenerComponent>     audioListener;
    std::optional<UIRectComponent>            uiRect;
    std::optional<UITextComponent>            uiText;
    std::optional<UIImageComponent>           uiImage;
    std::optional<UIButtonComponent>          uiButton;
    std::vector<ScriptInstance>               scripts;

    // Defined out-of-line so unique_ptr<Script> only needs the full type in Entity.cpp.
    Entity();
    ~Entity();
    Entity(const Entity& src);
    Entity& operator=(const Entity& src);
    Entity(Entity&& other) noexcept;
    Entity& operator=(Entity&& other) noexcept;
};

} // namespace hopf::scene
