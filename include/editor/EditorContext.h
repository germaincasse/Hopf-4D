#pragma once

#include "scene/Entity.h"

#include <optional>

namespace hopf::scene { class Scene; }

namespace hopf::editor {

struct EditorContext {
    scene::Scene*                scene    = nullptr;
    scene::EntityId              selected = 0;
    std::optional<scene::Entity> clipboard;
};

} // namespace hopf::editor
