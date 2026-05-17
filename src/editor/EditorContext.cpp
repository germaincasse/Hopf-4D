#include "editor/EditorContext.h"

#include "scene/Scene.h"

namespace hopf::editor {

namespace {
constexpr size_t kMaxUndo = 50;
}

void EditorContext::pushUndo() {
    if (!scene) return;
    undoStack.push_back(scene->entities()); // copy
    while (undoStack.size() > kMaxUndo) undoStack.pop_front();
}

void EditorContext::undo() {
    if (!scene || undoStack.empty()) return;
    scene->entities() = std::move(undoStack.back());
    undoStack.pop_back();
    if (selected != 0 && !scene->findEntity(selected)) selected = 0;
}

} // namespace hopf::editor
