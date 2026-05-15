#pragma once

#include "editor/panels/Panel.h"
#include "render/Camera4D.h"
#include "render/ViewportRenderer.h"

namespace hopf::editor {

class GameViewPanel : public Panel {
public:
    const char* name() const override { return "Game"; }
    void        render(EditorContext& ctx) override;

private:
    render::Camera4D         m_camera;
    render::ViewportRenderer m_renderer;
};

} // namespace hopf::editor
