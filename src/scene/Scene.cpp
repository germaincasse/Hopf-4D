#include "scene/Scene.h"

namespace hopf::scene {

const geometry::Mesh4D* Scene::addMesh(geometry::Mesh4D mesh) {
    auto owned = std::make_unique<geometry::Mesh4D>(std::move(mesh));
    const auto* ptr = owned.get();
    m_meshes.push_back(std::move(owned));
    return ptr;
}

Entity& Scene::addEntity(std::string name) {
    Entity e;
    e.id = m_nextId++;
    e.name = std::move(name);
    m_entities.push_back(std::move(e));
    return m_entities.back();
}

Entity& Scene::addPrimitive(geometry::PrimitiveType type, float size) {
    auto* mesh = addMesh(geometry::buildPrimitive(type, size));
    auto& e = addEntity(geometry::primitiveLabel(type));
    e.mesh = mesh;
    return e;
}

bool Scene::removeEntity(EntityId id) {
    for (auto it = m_entities.begin(); it != m_entities.end(); ++it) {
        if (it->id == id) {
            m_entities.erase(it);
            return true;
        }
    }
    return false;
}

void Scene::moveEntityBefore(EntityId src, EntityId target) {
    if (src == target) return;
    auto srcIt    = m_entities.end();
    auto targetIt = m_entities.end();
    for (auto it = m_entities.begin(); it != m_entities.end(); ++it) {
        if (it->id == src)    srcIt    = it;
        if (it->id == target) targetIt = it;
    }
    if (srcIt == m_entities.end() || targetIt == m_entities.end()) return;

    Entity moving = std::move(*srcIt);
    m_entities.erase(srcIt);
    // Re-find target since the iterator was invalidated.
    targetIt = m_entities.end();
    for (auto it = m_entities.begin(); it != m_entities.end(); ++it) {
        if (it->id == target) { targetIt = it; break; }
    }
    if (targetIt == m_entities.end()) {
        m_entities.push_back(std::move(moving));
    } else {
        m_entities.insert(targetIt, std::move(moving));
    }
}

Entity* Scene::findEntity(EntityId id) {
    for (auto& e : m_entities) if (e.id == id) return &e;
    return nullptr;
}

const Entity* Scene::findEntity(EntityId id) const {
    for (const auto& e : m_entities) if (e.id == id) return &e;
    return nullptr;
}

} // namespace hopf::scene
