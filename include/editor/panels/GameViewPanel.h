#pragma once

#include "editor/panels/Panel.h"
#include "render/Camera4D.h"
#include "render/ViewportRenderer.h"

#include <imgui.h>

namespace hopf::editor {

class GameViewPanel : public Panel {
public:
    const char* name() const override { return "Game"; }
    void        render(EditorContext& ctx) override;

private:
    render::Camera4D         m_camera;
    render::ViewportRenderer m_renderer;

    // Persistent 3D-orbit state for the Game view. The 4D-side of m_camera is
    // re-synced from the main camera entity each frame; these stay so the
    // player can navigate without the entity overriding them.
    bool   m_clickPendingL = false;
    bool   m_clickPendingR = false;
    ImVec2 m_clickStart{};
};

} // namespace hopf::editor
