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
    e.primitiveType = geometry::primitiveTypeName(type);
    return e;
}

Entity& Scene::duplicateEntity(const Entity& src, std::string name) {
    Entity copy = src;
    copy.id   = m_nextId++;
    copy.name = std::move(name);
    m_entities.push_back(std::move(copy));
    return m_entities.back();
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

void Scene::setParent(EntityId child, EntityId newParent) {
    if (child == 0 || child == newParent) return;
    auto* c = findEntity(child);
    if (!c) return;
    // Reject cycles: walking up newParent must not hit child.
    EntityId walk = newParent;
    while (walk != 0) {
        if (walk == child) return;
        const auto* p = findEntity(walk);
        if (!p) break;
        walk = p->parent;
    }
    c->parent = newParent;
}

void Scene::moveEntityToEnd(EntityId src) {
    auto it = m_entities.end();
    for (auto i = m_entities.begin(); i != m_entities.end(); ++i) {
        if (i->id == src) { it = i; break; }
    }
    if (it == m_entities.end()) return;
    Entity moving = std::move(*it);
    m_entities.erase(it);
    m_entities.push_back(std::move(moving));
}

void Scene::clear() {
    m_entities.clear();
    m_meshes.clear();
    m_nextId = 1;
}

void Scene::refreshNextIdFromContents() {
    EntityId maxId = 0;
    for (const auto& e : m_entities) if (e.id > maxId) maxId = e.id;
    m_nextId = maxId + 1;
}

math::Mat5 Scene::worldMatrix(EntityId id) const {
    const auto* e = findEntity(id);
    if (!e) return math::Mat5::identity();
    math::Mat5 m = e->transform.toMatrix();
    EntityId pid = e->parent;
    // Safety against accidental cycles: cap at depth 64.
    int guard = 64;
    while (pid != 0 && guard-- > 0) {
        const auto* p = findEntity(pid);
        if (!p) break;
        m = p->transform.toMatrix() * m;
        pid = p->parent;
    }
    return m;
}

} // namespace hopf::scene
