#include "scripting/Script.h"

#include "scene/Scene.h"

namespace hopf::scripting {

scene::Entity* Script::entity() {
    return m_scene ? m_scene->findEntity(m_entityId) : nullptr;
}

const scene::Entity* Script::entity() const {
    return m_scene ? m_scene->findEntity(m_entityId) : nullptr;
}

} // namespace hopf::scripting
