#include "scene/Entity.h"

#include "scripting/Script.h"
#include "scripting/ScriptRegistry.h"

namespace hopf::scene {

Entity::Entity()                              = default;
Entity::~Entity()                             = default;
Entity::Entity(Entity&&) noexcept             = default;
Entity& Entity::operator=(Entity&&) noexcept  = default;

Entity::Entity(const Entity& src) { *this = src; }

Entity& Entity::operator=(const Entity& src) {
    if (this == &src) return *this;

    id            = src.id;
    parent        = src.parent;
    name          = src.name;
    transform     = src.transform;
    mesh          = src.mesh;
    primitiveType = src.primitiveType;
    visible       = src.visible;
    autoRotate    = src.autoRotate;
    light         = src.light;
    camera2D      = src.camera2D;
    camera3D      = src.camera3D;
    camera4D      = src.camera4D;
    collider      = src.collider;
    rigidbody     = src.rigidbody;
    tint          = src.tint;
    audioSource   = src.audioSource;
    audioListener = src.audioListener;
    uiRect        = src.uiRect;
    uiText        = src.uiText;
    uiImage       = src.uiImage;
    uiButton      = src.uiButton;

    // Re-instantiate scripts from the registry so that duplicates and play/stop
    // snapshots get fresh, independent state. If a registered factory is missing
    // (e.g. the binary no longer contains that script type) we drop the entry.
    scripts.clear();
    scripts.reserve(src.scripts.size());
    auto& reg = scripting::ScriptRegistry::instance();
    for (const auto& s : src.scripts) {
        ScriptInstance ns;
        ns.typeName = s.typeName;
        ns.instance = reg.create(s.typeName);
        if (ns.instance) scripts.push_back(std::move(ns));
    }
    return *this;
}

} // namespace hopf::scene
