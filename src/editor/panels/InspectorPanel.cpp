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

    if (entity->light.has_value()) {
        if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextDisabled("Direction = transform.rotation applied to (0, -1, 0, 0).");
            ImGui::DragFloat("Intensity##light", &entity->light->intensity, 0.05f, 0.f, 10.f);
            float col[3] = {entity->light->r, entity->light->g, entity->light->b};
            if (ImGui::ColorEdit3("Color##light", col, ImGuiColorEditFlags_PickerHueWheel)) {
                entity->light->r = col[0];
                entity->light->g = col[1];
                entity->light->b = col[2];
            }
            if (ImGui::Button("Remove light")) entity->light.reset();
        }
    }

    if (entity->camera2D.has_value()) {
        if (ImGui::CollapsingHeader("Camera 2D", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat("Ortho size##c2d", &entity->camera2D->orthoSize, 0.05f, 0.1f, 100.f);
            ImGui::DragFloat("Near##c2d",       &entity->camera2D->zNear, 0.5f);
            ImGui::DragFloat("Far##c2d",        &entity->camera2D->zFar,  0.5f);
            ImGui::ColorEdit3("Background##c2d", &entity->camera2D->bg.r);
            if (ImGui::Button("Remove##c2d")) entity->camera2D.reset();
        }
    }
    if (entity->camera3D.has_value()) {
        if (ImGui::CollapsingHeader("Camera 3D", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Perspective##c3d", &entity->camera3D->perspective);
            if (entity->camera3D->perspective) {
                ImGui::DragFloat("Fov Y (deg)##c3d", &entity->camera3D->fovYDeg, 0.5f, 5.f, 170.f);
            } else {
                ImGui::DragFloat("Ortho half-H##c3d", &entity->camera3D->orthoHalfH, 0.05f, 0.1f, 100.f);
            }
            ImGui::DragFloat("Near##c3d", &entity->camera3D->zNear, 0.01f, 0.001f, 100.f);
            ImGui::DragFloat("Far##c3d",  &entity->camera3D->zFar,  1.0f,  1.f,    10000.f);
            ImGui::ColorEdit3("Background##c3d", &entity->camera3D->bg.r);
            if (ImGui::Button("Remove##c3d")) entity->camera3D.reset();
        }
    }
    if (entity->camera4D.has_value()) {
        if (ImGui::CollapsingHeader("Camera 4D", ImGuiTreeNodeFlags_DefaultOpen)) {
            const char* modes[] = { "Projection", "Slice" };
            int m = static_cast<int>(entity->camera4D->mode);
            if (ImGui::Combo("Mode##c4d", &m, modes, IM_ARRAYSIZE(modes))) {
                entity->camera4D->mode = static_cast<scene::Camera4DComponent::Mode>(m);
            }
            if (entity->camera4D->mode == scene::Camera4DComponent::Mode::Projection) {
                ImGui::Checkbox("Perspective 4D##c4d", &entity->camera4D->perspective4);
                if (entity->camera4D->perspective4) {
                    ImGui::DragFloat("Focal4##c4d",  &entity->camera4D->focal4,  0.05f, 0.5f, 20.f);
                    ImGui::DragFloat("wOffset##c4d", &entity->camera4D->wOffset, 0.05f, -10.f, 10.f);
                }
            } else {
                const char* axes[] = { "X", "Y", "Z", "W" };
                ImGui::Combo("Slice axis##c4d", &entity->camera4D->sliceAxis, axes, IM_ARRAYSIZE(axes));
                ImGui::DragFloat("Slice value##c4d", &entity->camera4D->sliceVal, 0.01f, -2.f, 2.f);
            }
            ImGui::Checkbox("Main camera (Game view)##c4d", &entity->camera4D->isMain);
            ImGui::ColorEdit3("Background##c4d", &entity->camera4D->bg.r);
            if (ImGui::Button("Remove##c4d")) entity->camera4D.reset();
        }
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

    if (entity->collider.has_value()) {
        if (ImGui::CollapsingHeader("Mesh Collider", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Dimension: %dD", entity->collider->dimension);
            ImGui::Checkbox("Is Trigger##coll", &entity->collider->isTrigger);
            if (ImGui::Button("Remove##coll")) entity->collider.reset();
        }
    }
    if (entity->rigidbody.has_value()) {
        if (ImGui::CollapsingHeader("Rigidbody", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Dimension: %dD", entity->rigidbody->dimension);
            ImGui::DragFloat("Mass##rb",     &entity->rigidbody->mass, 0.05f, 0.f, 1e6f);
            ImGui::Checkbox("Kinematic##rb", &entity->rigidbody->kinematic);
            ImGui::Checkbox("Use gravity##rb", &entity->rigidbody->useGravity);
            ImGui::TextDisabled("Velocity");
            dragVec4Axes("rb_vel", entity->rigidbody->velocity, 0.05f);
            if (ImGui::Button("Remove##rb")) entity->rigidbody.reset();
        }
    }

    ImGui::Separator();
    const float wAvail = ImGui::GetContentRegionAvail().x;
    if (ImGui::Button("Add Component", ImVec2(wAvail, 0))) {
        ImGui::OpenPopup("##addComponentPopup");
    }
    if (ImGui::BeginPopup("##addComponentPopup")) {
        const int dim = entity->mesh ? scene::meshDimension(*entity->mesh) : 4;
        ImGui::TextDisabled("Detected dimension: %dD", dim);
        ImGui::Separator();
        if (!entity->collider.has_value()) {
            if (ImGui::MenuItem("Mesh Collider")) {
                scene::MeshColliderComponent c;
                c.dimension = dim;
                entity->collider = c;
            }
        }
        if (!entity->rigidbody.has_value()) {
            if (ImGui::MenuItem("Rigidbody")) {
                scene::RigidbodyComponent rb;
                rb.dimension = dim;
                entity->rigidbody = rb;
            }
        }
        if (!entity->light.has_value()) {
            if (ImGui::MenuItem("Directional Light")) {
                entity->light = scene::DirectionalLight{};
            }
        }
        if (!entity->camera2D.has_value() && !entity->camera3D.has_value() && !entity->camera4D.has_value()) {
            if (ImGui::MenuItem("Camera 2D")) entity->camera2D = scene::Camera2DComponent{};
            if (ImGui::MenuItem("Camera 3D")) entity->camera3D = scene::Camera3DComponent{};
            if (ImGui::MenuItem("Camera 4D")) {
                entity->camera4D = scene::Camera4DComponent{};
                bool anyMain = false;
                for (const auto& other : ctx.scene->entities()) {
                    if (other.id != entity->id && other.camera4D.has_value() && other.camera4D->isMain) {
                        anyMain = true;
                        break;
                    }
                }
                if (!anyMain) entity->camera4D->isMain = true;
            }
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

} // namespace hopf::editor
