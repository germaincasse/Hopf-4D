#pragma once

#include "editor/panels/Panel.h"
#include "math/Rotor4.h"
#include "math/Vec4.h"
#include "render/Camera4D.h"
#include "render/ViewportRenderer.h"
#include "scene/Entity.h"

#include <imgui.h>

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
    struct GizmoDrag {
        bool             active   = false;
        scene::EntityId  entityId = 0;
        ToolMode         mode     = ToolMode::Hand;
        int              handle   = -1; // 0..3 (axes) or 0..5 (rotation planes) or 0..4 (scale + uniform)
        ImVec2           mouseStart{};
        math::Vec4       initialPos{};      // local position at drag start
        math::Vec4       initialWorldPos{}; // world position at drag start (used for screen projection)
        math::Vec4       initialScale{1.f, 1.f, 1.f, 1.f};
        math::Rotor4     initialRot{};
    };

    int                      m_id = 0;
    std::string              m_name;
    render::Camera4D         m_camera;
    render::ViewportRenderer m_renderer;
    bool                     m_clickPending = false;
    GizmoDrag                m_drag{};

    // Smooth focus-target state: when the user presses F on a selected entity, the
    // pivot and distance lerp toward (entity world pos, close/far). Repeated F
    // presses toggle between a close and a far distance.
    bool   m_focusActive  = false;
    bool   m_focusFar     = false;
    float  m_focusPivotX  = 0.f;
    float  m_focusPivotY  = 0.f;
    float  m_focusPivotZ  = 0.f;
    float  m_focusDistance = 5.f;
};

} // namespace hopf::editor
