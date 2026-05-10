#pragma once

#include "editor/panels/Panel.h"

namespace hopf::editor {

class InspectorPanel : public Panel {
public:
    const char* name() const override { return "Inspector"; }
    void render(EditorContext& ctx) override;
};

} // namespace hopf::editor
