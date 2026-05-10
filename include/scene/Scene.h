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
    bool    removeEntity(EntityId id);
    void    moveEntityBefore(EntityId src, EntityId target);

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
