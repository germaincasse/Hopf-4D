#include "editor/panels/GameViewPanel.h"

#include "scene/Scene.h"

#include <imgui.h>

namespace hopf::editor {

namespace {

const scene::Entity* findMainCamera(const scene::Scene& scene) {
    for (const auto& e : scene.entities()) {
        if (e.camera4D.has_value() && e.camera4D->isMain) return &e;
    }
    return nullptr;
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

    const scene::Entity* cam = findMainCamera(*ctx.scene);
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
    m_camera.projectionStyle = c4.perspective4 ? render::ProjectionStyle::Perspective
                                               : render::ProjectionStyle::Isometric;
    m_camera.focal4    = c4.focal4;
    m_camera.wOffset   = c4.wOffset;
    m_camera.sliceAxis = static_cast<render::SliceAxis>(c4.sliceAxis);
    m_camera.sliceVal  = c4.sliceVal;
    m_camera.style     = render::DisplayStyle::SolidLit;
    m_camera.grid      = render::GridSettings{};
    m_camera.grid.showXZ = false;
    m_camera.bgR = c4.bg.r;
    m_camera.bgG = c4.bg.g;
    m_camera.bgB = c4.bg.b;

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x > 1.f && avail.y > 1.f) {
        m_renderer.resize(static_cast<int>(avail.x), static_cast<int>(avail.y));
        m_renderer.render(*ctx.scene, m_camera);
        ImGui::Image(static_cast<ImTextureID>(m_renderer.colorTexture()),
                     ImVec2(static_cast<float>(m_renderer.width()),
                            static_cast<float>(m_renderer.height())),
                     ImVec2(0, 1), ImVec2(1, 0));
    }

    ImGui::End();
}

} // namespace hopf::editor
