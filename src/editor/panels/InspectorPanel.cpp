#include "hopf/editor/panels/InspectorPanel.h"

#include "hopf/scene/Scene.h"

#include <imgui.h>

#include <cstdio>

namespace hopf::editor {

void InspectorPanel::render(EditorContext& ctx) {
    if (!m_open) return;
    if (!ImGui::Begin(name(), &m_open)) {
        ImGui::End();
        return;
    }

    if (!ctx.scene) {
        ImGui::TextDisabled("No scene loaded.");
        ImGui::End();
        return;
    }

    auto* entity = ctx.scene->findEntity(ctx.selected);
    if (!entity) {
        ImGui::TextDisabled("No entity selected.");
        ImGui::End();
        return;
    }

    char nameBuf[128];
    std::snprintf(nameBuf, sizeof(nameBuf), "%s", entity->name.c_str());
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
        entity->name = nameBuf;
    }
    ImGui::Checkbox("Visible", &entity->visible);

    if (ImGui::CollapsingHeader("Transform 4D", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat4("Position", &entity->transform.position.x, 0.05f);
        ImGui::DragFloat4("Scale",    &entity->transform.scale.x,    0.05f, 0.01f, 10.f);

        ImGui::SeparatorText("Rotation (rad, per plane)");
        ImGui::DragFloat("xy", &entity->transform.rotation.xy, 0.01f);
        ImGui::DragFloat("xz", &entity->transform.rotation.xz, 0.01f);
        ImGui::DragFloat("xw", &entity->transform.rotation.xw, 0.01f);
        ImGui::DragFloat("yz", &entity->transform.rotation.yz, 0.01f);
        ImGui::DragFloat("yw", &entity->transform.rotation.yw, 0.01f);
        ImGui::DragFloat("zw", &entity->transform.rotation.zw, 0.01f);
    }

    if (ImGui::CollapsingHeader("Auto-rotate (rad/s, per plane)")) {
        if (ImGui::Button("Reset")) {
            entity->autoRotate = {};
        }
        ImGui::DragFloat("xy##auto", &entity->autoRotate.xy, 0.01f, -5.f, 5.f);
        ImGui::DragFloat("xz##auto", &entity->autoRotate.xz, 0.01f, -5.f, 5.f);
        ImGui::DragFloat("xw##auto", &entity->autoRotate.xw, 0.01f, -5.f, 5.f);
        ImGui::DragFloat("yz##auto", &entity->autoRotate.yz, 0.01f, -5.f, 5.f);
        ImGui::DragFloat("yw##auto", &entity->autoRotate.yw, 0.01f, -5.f, 5.f);
        ImGui::DragFloat("zw##auto", &entity->autoRotate.zw, 0.01f, -5.f, 5.f);
    }

    if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (entity->mesh) {
            ImGui::Text("name      : %s", entity->mesh->name.c_str());
            ImGui::Text("vertices  : %zu", entity->mesh->vertices.size());
            ImGui::Text("edges     : %zu", entity->mesh->edges.size());
            ImGui::Text("tetrahedra: %zu", entity->mesh->tetrahedra.size());
        } else {
            ImGui::TextDisabled("(none)");
        }
    }

    ImGui::End();
}

} // namespace hopf::editor
