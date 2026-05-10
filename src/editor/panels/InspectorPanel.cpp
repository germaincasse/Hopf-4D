#include "editor/panels/InspectorPanel.h"

#include "editor/AxisColors.h"
#include "scene/Scene.h"

#include <imgui.h>

#include <algorithm>
#include <cstdio>

namespace hopf::editor {

namespace {

bool dragVec4Axes(const char* idPrefix, math::Vec4& v, float speed,
                  float minV = 0.f, float maxV = 0.f, const char* fmt = "%.3f")
{
    constexpr char letters[4] = {'X', 'Y', 'Z', 'W'};
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float labelW  = ImGui::CalcTextSize("X").x;
    const float availW  = ImGui::GetContentRegionAvail().x;
    const float fieldW  = std::max(40.f, (availW - 4.f * (labelW + 2.f * spacing)) / 4.f);

    bool changed = false;
    for (int i = 0; i < 4; ++i) {
        ImGui::PushStyleColor(ImGuiCol_Text, axisColor(letters[i]));
        const char buf[2] = {letters[i], '\0'};
        ImGui::TextUnformatted(buf);
        ImGui::PopStyleColor();
        ImGui::SameLine();

        char id[16];
        std::snprintf(id, sizeof(id), "##%s_%d", idPrefix, i);
        ImGui::SetNextItemWidth(fieldW);
        if (ImGui::DragFloat(id, &v[i], speed, minV, maxV, fmt)) changed = true;
        if (i < 3) ImGui::SameLine();
    }
    return changed;
}

bool dragPlaneAxes(const char* plane, const char* idPrefix, float& val, float speed,
                   float minV = 0.f, float maxV = 0.f)
{
    textAxisColored(plane);
    ImGui::SameLine();
    char id[24];
    std::snprintf(id, sizeof(id), "##%s_%s", idPrefix, plane);
    return ImGui::DragFloat(id, &val, speed, minV, maxV);
}

} // namespace

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
        ImGui::TextUnformatted("Position");
        dragVec4Axes("pos", entity->transform.position, 0.05f);

        ImGui::TextUnformatted("Scale");
        dragVec4Axes("scale", entity->transform.scale, 0.05f, 0.01f, 10.f);

        ImGui::SeparatorText("Rotation (rad, per plane)");
        dragPlaneAxes("xy", "rot", entity->transform.rotation.xy, 0.01f);
        dragPlaneAxes("xz", "rot", entity->transform.rotation.xz, 0.01f);
        dragPlaneAxes("xw", "rot", entity->transform.rotation.xw, 0.01f);
        dragPlaneAxes("yz", "rot", entity->transform.rotation.yz, 0.01f);
        dragPlaneAxes("yw", "rot", entity->transform.rotation.yw, 0.01f);
        dragPlaneAxes("zw", "rot", entity->transform.rotation.zw, 0.01f);
    }

    if (ImGui::CollapsingHeader("Auto-rotate (rad/s, per plane)")) {
        if (ImGui::Button("Reset")) {
            entity->autoRotate = {};
        }
        dragPlaneAxes("xy", "auto", entity->autoRotate.xy, 0.01f, -5.f, 5.f);
        dragPlaneAxes("xz", "auto", entity->autoRotate.xz, 0.01f, -5.f, 5.f);
        dragPlaneAxes("xw", "auto", entity->autoRotate.xw, 0.01f, -5.f, 5.f);
        dragPlaneAxes("yz", "auto", entity->autoRotate.yz, 0.01f, -5.f, 5.f);
        dragPlaneAxes("yw", "auto", entity->autoRotate.yw, 0.01f, -5.f, 5.f);
        dragPlaneAxes("zw", "auto", entity->autoRotate.zw, 0.01f, -5.f, 5.f);
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
