#pragma once

// Base class for user-written gameplay scripts. Attach an instance to an entity
// via the editor (or in code through Scene::addEntity(...).scripts.push_back).
// All members are virtual no-ops so users override only what they need, similar
// to Unity's MonoBehaviour callbacks.
//
// Scripts must not call into OpenGL, ImGui, or any low-level engine subsystem
// directly. Use the accessors below (entity, scene) plus the math/scene types.

#include "math/Vec4.h"

#include <string>

namespace hopf::scene { class Scene; struct Entity; using EntityId = uint64_t; }

namespace hopf::scripting {

class Script {
public:
    virtual ~Script() = default;

    // Unique type identifier (matches HOPF_REGISTER_SCRIPT macro).
    virtual const char* typeName() const = 0;

    // Called once when play starts (or when the script is added during play).
    virtual void onStart() {}

    // Called every frame during play with the frame's delta time in seconds.
    virtual void onUpdate(float /*dt*/) {}

    // Generic message hook used by UI buttons (`onClickMethod`) and by other
    // scripts wanting loose coupling. Override and dispatch on the method name.
    virtual void onMessage(const std::string& /*method*/) {}

    // Fired in Play mode when the player clicks an entity in the Game view.
    // `button` is 0 for left, 1 for right. The handler runs on EVERY script in
    // the scene, so check entity()->id == clicked or compare yourself.
    virtual void onEntityClicked(scene::EntityId /*clicked*/, int /*button*/) {}

    // Fired in Play mode when the mouse wheel scrolls inside the Game view.
    virtual void onMouseWheel(float /*delta*/) {}

    // Fired in Play mode each frame the mouse is being dragged inside the Game
    // view. `button` is 0 for left, 1 for right, 2 for middle. `dx`/`dy` are the
    // pixel delta since the previous frame.
    virtual void onMouseDragged(float /*dx*/, float /*dy*/, int /*button*/) {}

    // Fired in Play mode on mouse press / release inside the Game view. Use this
    // for click-and-hold mechanics (charging a shot, holding to aim, etc).
    // `button` is 0 for left, 1 for right, 2 for middle.
    virtual void onMouseButton(int /*button*/, bool /*pressed*/) {}

    // Owning entity / scene. Re-resolved each call so the pointer stays valid even
    // when other scripts spawn or remove entities (which would invalidate a stored
    // Entity*). Returns nullptr if the owning entity has been destroyed.
    scene::Entity*       entity();
    const scene::Entity* entity() const;
    scene::Scene*        scene()        { return m_scene; }
    const scene::Scene*  scene() const  { return m_scene; }

    scene::EntityId      entityId() const { return m_entityId; }

    // Internal: bind to a runtime context. Called by the engine, not by user code.
    void _bind(scene::EntityId id, scene::Scene* s) { m_entityId = id; m_scene = s; }

private:
    scene::EntityId m_entityId = 0;
    scene::Scene*   m_scene    = nullptr;
};

} // namespace hopf::scripting
