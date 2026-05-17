#pragma once

#include "scene/Entity.h"

#include <deque>
#include <optional>
#include <vector>

namespace hopf::scene { class Scene; }

namespace hopf::editor {

enum class PlayState { Stopped, Playing, Paused };
enum class ToolMode  { Hand, Translate, Rotate, Scale };

struct EditorContext {
    scene::Scene*                scene    = nullptr;
    scene::EntityId              selected = 0;
    std::optional<scene::Entity> clipboard;
    PlayState                    playState = PlayState::Stopped;
    ToolMode                     toolMode  = ToolMode::Translate;

    // Undo stack: each entry is a full snapshot of the scene's entities. Capped to
    // 50 entries; the oldest is dropped when the limit is reached. Push before any
    // user-driven mutation (drag start, add/remove/duplicate, reparent, ...).
    std::deque<std::vector<scene::Entity>> undoStack;

    // Snapshots the scene's entities for a future undo. No-op if scene is null.
    void pushUndo();

    // Restores the most recent snapshot, dropping it. No-op if the stack is empty.
    void undo();
};

} // namespace hopf::editor
