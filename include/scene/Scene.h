#pragma once

#include "geometry/Mesh4D.h"
#include "geometry/Primitives.h"
#include "scene/Entity.h"

#include <memory>
#include <string>
#include <vector>

namespace hopf::scene {

class Scene {
public:
    Scene() = default;

    void               setName(std::string n) { m_name = std::move(n); }
    const std::string& name() const           { return m_name; }

    const geometry::Mesh4D* addMesh(geometry::Mesh4D mesh);

    Entity& addEntity(std::string name);
    Entity& addPrimitive(geometry::PrimitiveType type, float size = 1.f);
    Entity& duplicateEntity(const Entity& src, std::string name);
    bool    removeEntity(EntityId id);
    void    moveEntityBefore(EntityId src, EntityId target);

    // Set or clear (newParent == 0) the parent of an entity. No-op if the change
    // would introduce a cycle.
    void    setParent(EntityId child, EntityId newParent);

    // World transform of an entity, walking up parents. Returns identity if the
    // entity doesn't exist.
    math::Mat5 worldMatrix(EntityId id) const;

    // Drops all entities and meshes, resets the id counter. Used before loading.
    void    clear();

    // After bulk-loading entities (with preserved IDs) into entities(), call this
    // so that subsequent addEntity()/addPrimitive() pick a unique next id.
    void    refreshNextIdFromContents();

    // Registers a mesh built externally (e.g. from a serializer that already created
    // it). The returned pointer is owned by the scene.
    const geometry::Mesh4D* takeOwnedMesh(geometry::Mesh4D mesh) { return addMesh(std::move(mesh)); }

    std::vector<Entity>&       entities()       { return m_entities; }
    const std::vector<Entity>& entities() const { return m_entities; }

    Entity*       findEntity(EntityId id);
    const Entity* findEntity(EntityId id) const;

private:
    std::string                                    m_name = "Untitled";
    std::vector<std::unique_ptr<geometry::Mesh4D>> m_meshes;
    std::vector<Entity>                            m_entities;
    EntityId                                       m_nextId = 1;
};

} // namespace hopf::scene
