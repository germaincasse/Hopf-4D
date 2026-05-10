#pragma once

#include "editor/panels/Panel.h"

namespace hopf::editor {

class ConsolePanel : public Panel {
public:
    const char* name() const override { return "Console"; }
    void render(EditorContext& ctx) override;
};

} // namespace hopf::editor
