#include "editor/panels/GameViewPanel.h"

#include "scene/Scene.h"
#include "scripting/Script.h"
#include "ui/UIRenderer.h"

#include <imgui.h>

namespace hopf::editor {

namespace {

scene::Entity* findMainCamera(scene::Scene& scene) {
    for (auto& e : scene.entities()) {
        if (e.camera4D.has_value() && e.camera4D->isMain) return &e;
    }
    return nullptr;
}

void dispatchEntityClicked(scene::Scene& scene, scene::EntityId clicked, int button) {
    for (auto& e : scene.entities()) {
        for (auto& s : e.scripts) {
            if (s.instance) s.instance->onEntityClicked(clicked, button);
        }
    }
}

void dispatchMouseWheel(scene::Scene& scene, float delta) {
    for (auto& e : scene.entities()) {
        for (auto& s : e.scripts) {
            if (s.instance) s.instance->onMouseWheel(delta);
        }
    }
}

} // namespace

void GameViewPanel::render(EditorContext& ctx) {
    if (!m_open) return;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (!ImGui::Begin(name(), &m_open)) {
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }
    ImGui::PopStyleVar();

    if (!ctx.scene) {
        ImGui::TextDisabled("No scene.");
        ImGui::End();
        return;
    }

    scene::Entity* cam = findMainCamera(*ctx.scene);
    if (!cam) {
        ImGui::TextDisabled("No main Camera 4D in the scene.");
        ImGui::TextDisabled("Add a Camera 4D component and tick \"Main camera (Game view)\".");
        ImGui::End();
        return;
    }

    const auto& c4 = *cam->camera4D;
    m_camera.position4 = cam->transform.position;
    m_camera.rotation4 = cam->transform.rotation;
    m_camera.mode      = (c4.mode == scene::Camera4DComponent::Mode::Slice)
                            ? render::ViewMode::Slice
                            : render::ViewMode::Projection;
    switch (c4.projection) {
        case scene::CameraProjection::Isometric:     m_camera.projectionStyle = render::ProjectionStyle::Isometric;   break;
        case scene::CameraProjection::Perspective3D: m_camera.projectionStyle = render::ProjectionStyle::Hybrid;      break;
        case scene::CameraProjection::Perspective4D: m_camera.projectionStyle = render::ProjectionStyle::Perspective; break;
    }
    m_camera.focal4    = c4.focal4;
    m_camera.wOffset   = c4.wOffset;
    m_camera.sliceAxis = c4.sliceAxis;
    m_camera.sliceVal  = c4.sliceVal;
    switch (c4.displayStyle) {
        case scene::CameraDisplayStyle::Wire:  m_camera.style = render::DisplayStyle::SimpleWireframe; break;
        case scene::CameraDisplayStyle::Depth: m_camera.style = render::DisplayStyle::DepthWireframe;  break;
        case scene::CameraDisplayStyle::Unlit: m_camera.style = render::DisplayStyle::SolidUnlit;      break;
        case scene::CameraDisplayStyle::Lit:   m_camera.style = render::DisplayStyle::SolidLit;        break;
    }
    m_camera.grid          = render::GridSettings{};
    m_camera.grid.showXZ   = false;
    m_camera.bgR           = c4.bg.r;
    m_camera.bgG           = c4.bg.g;
    m_camera.bgB           = c4.bg.b;
    m_camera.showWorldAxes = false;

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x > 1.f && avail.y > 1.f) {
        m_renderer.resize(static_cast<int>(avail.x), static_cast<int>(avail.y));
        m_renderer.render(*ctx.scene, m_camera);
        ImGui::Image(static_cast<ImTextureID>(m_renderer.colorTexture()),
                     ImVec2(static_cast<float>(m_renderer.width()),
                            static_cast<float>(m_renderer.height())),
                     ImVec2(0, 1), ImVec2(1, 0));

        const ImVec2 imgMin   = ImGui::GetItemRectMin();
        const ImVec2 imgSize  = ImGui::GetItemRectSize();
        const bool   hovered  = ImGui::IsItemHovered();
        const ImGuiIO& io     = ImGui::GetIO();
        const bool   playing  = (ctx.playState == PlayState::Playing);

        // The Game view is intentionally a passive display of what the main camera
        // sees -- no orbit / pan / wheel-zoom of the camera. Mouse events only fire
        // gameplay callbacks during Play mode.
        if (hovered && playing) {
            if (io.MouseWheel != 0.f) dispatchMouseWheel(*ctx.scene, io.MouseWheel);

            auto handleClick = [&](int btn, bool& pending) {
                if (ImGui::IsMouseClicked(btn)) { pending = true; }
                if (pending && ImGui::IsMouseDragging(btn, 5.f)) pending = false;
                if (pending && ImGui::IsMouseReleased(btn)) {
                    pending = false;
                    const float u = io.MousePos.x - imgMin.x;
                    const float v = io.MousePos.y - imgMin.y;
                    const auto picked = m_renderer.pickEntityAt(*ctx.scene, m_camera, u, v);
                    if (picked != 0) dispatchEntityClicked(*ctx.scene, picked, btn);
                }
            };
            handleClick(ImGuiMouseButton_Left,  m_clickPendingL);
            handleClick(ImGuiMouseButton_Right, m_clickPendingR);
        } else {
            m_clickPendingL = false;
            m_clickPendingR = false;
        }

        ui::RenderOptions opts;
        opts.interactive = hovered && playing;
        ui::render(*ctx.scene, imgMin.x, imgMin.y, imgSize.x, imgSize.y,
                   ImGui::GetWindowDrawList(), opts);
    }

    ImGui::End();
}

} // namespace hopf::editor
