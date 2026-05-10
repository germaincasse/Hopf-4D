#pragma once

#include "editor/panels/Panel.h"
#include "render/Camera4D.h"
#include "render/ViewportRenderer.h"

#include <string>

namespace hopf::editor {

class ViewportPanel : public Panel {
public:
    explicit ViewportPanel(int id = 0);

    const char* name() const override { return m_name.c_str(); }
    void        render(EditorContext& ctx) override;

    render::Camera4D&       camera()       { return m_camera; }
    const render::Camera4D& camera() const { return m_camera; }

private:
    int                      m_id = 0;
    std::string              m_name;
    render::Camera4D         m_camera;
    render::ViewportRenderer m_renderer;
    bool                     m_clickPending = false;
};

} // namespace hopf::editor
