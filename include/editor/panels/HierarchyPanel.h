#pragma once

#include "editor/panels/Panel.h"

namespace hopf::editor {

class HierarchyPanel : public Panel {
public:
    const char* name() const override { return "Hierarchy"; }
    void render(EditorContext& ctx) override;
};

} // namespace hopf::editor
