#pragma once

#include "hopf/editor/panels/Panel.h"
#include "hopf/render/Camera4D.h"
#include "hopf/render/ViewportRenderer.h"

namespace hopf::editor {

class ViewportPanel : public Panel {
public:
    const char* name() const override { return "Viewport (4D)"; }
    void render(EditorContext& ctx) override;

    render::Camera4D&        camera()       { return m_camera; }
    const render::Camera4D&  camera() const { return m_camera; }

private:
    render::Camera4D       m_camera;
    render::ViewportRenderer m_renderer;
};

} // namespace hopf::editor
