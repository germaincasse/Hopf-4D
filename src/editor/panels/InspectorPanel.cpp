#include "editor/panels/InspectorPanel.h"

#include "audio/AudioEngine.h"
#include "editor/AxisColors.h"
#include "scene/Scene.h"
#include "scripting/ScriptRegistry.h"

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
            const char* styleNames[] = { "Wire", "Depth", "Unlit", "Lit" };
            int ds = static_cast<int>(entity->camera2D->displayStyle);
            if (ImGui::Combo("Display##c2d", &ds, styleNames, IM_ARRAYSIZE(styleNames))) {
                entity->camera2D->displayStyle = static_cast<scene::CameraDisplayStyle>(ds);
            }
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
            const char* styleNames[] = { "Wire", "Depth", "Unlit", "Lit" };
            int ds = static_cast<int>(entity->camera3D->displayStyle);
            if (ImGui::Combo("Display##c3d", &ds, styleNames, IM_ARRAYSIZE(styleNames))) {
                entity->camera3D->displayStyle = static_cast<scene::CameraDisplayStyle>(ds);
            }
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
            auto& c4 = *entity->camera4D;
            if (c4.mode == scene::Camera4DComponent::Mode::Projection) {
                // Three mutually exclusive projection presets as buttons.
                auto projBtn = [&](const char* label, scene::CameraProjection val, const char* tip) {
                    const bool active = (c4.projection == val);
                    if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                    if (ImGui::Button(label)) c4.projection = val;
                    if (active) ImGui::PopStyleColor();
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) ImGui::SetTooltip("%s", tip);
                };
                projBtn("Iso##c4d",   scene::CameraProjection::Isometric,
                        "Isometric: parallel in both 3D and 4D");
                ImGui::SameLine();
                projBtn("3D persp##c4d", scene::CameraProjection::Perspective3D,
                        "3D perspective on screen, parallel 4D collapse");
                ImGui::SameLine();
                projBtn("4D persp##c4d", scene::CameraProjection::Perspective4D,
                        "Full perspective in both 3D and 4D");
                if (c4.projection == scene::CameraProjection::Perspective4D) {
                    ImGui::DragFloat("Focal4##c4d",  &c4.focal4,  0.05f, 0.5f, 20.f);
                    ImGui::DragFloat("wOffset##c4d", &c4.wOffset, 0.05f, -10.f, 10.f);
                }
            } else {
                ImGui::TextDisabled("Slice normal (Vec4):");
                dragVec4Axes("c4d_slice", c4.sliceAxis, 0.02f);
                ImGui::SameLine();
                if (ImGui::SmallButton("X##c4dsl")) c4.sliceAxis = {1, 0, 0, 0};
                ImGui::SameLine();
                if (ImGui::SmallButton("Y##c4dsl")) c4.sliceAxis = {0, 1, 0, 0};
                ImGui::SameLine();
                if (ImGui::SmallButton("Z##c4dsl")) c4.sliceAxis = {0, 0, 1, 0};
                ImGui::SameLine();
                if (ImGui::SmallButton("W##c4dsl")) c4.sliceAxis = {0, 0, 0, 1};
                ImGui::DragFloat("Slice offset##c4d", &c4.sliceVal, 0.01f, -10.f, 10.f);
            }
            const char* styleNames[] = { "Wire", "Depth", "Unlit", "Lit" };
            int ds = static_cast<int>(entity->camera4D->displayStyle);
            if (ImGui::Combo("Display##c4d", &ds, styleNames, IM_ARRAYSIZE(styleNames))) {
                entity->camera4D->displayStyle = static_cast<scene::CameraDisplayStyle>(ds);
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
    if (entity->tint.has_value()) {
        if (ImGui::CollapsingHeader("Color Tint", ImGuiTreeNodeFlags_DefaultOpen)) {
            float col[3] = {entity->tint->r, entity->tint->g, entity->tint->b};
            if (ImGui::ColorEdit3("Tint##tint", col)) {
                entity->tint->r = col[0];
                entity->tint->g = col[1];
                entity->tint->b = col[2];
            }
            if (ImGui::Button("Remove##tint")) entity->tint.reset();
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
    if (entity->audioSource.has_value()) {
        if (ImGui::CollapsingHeader("Audio Source", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& a = *entity->audioSource;
            char buf[256]; std::snprintf(buf, sizeof(buf), "%s", a.clipPath.c_str());
            if (ImGui::InputText("Clip##as", buf, sizeof(buf))) a.clipPath = buf;
            ImGui::DragFloat("Volume##as",  &a.volume,      0.01f, 0.f, 2.f);
            ImGui::DragFloat("Pitch##as",   &a.pitch,       0.01f, 0.1f, 4.f);
            ImGui::Checkbox("Loop##as",         &a.loop);
            ImGui::Checkbox("Play on start##as",&a.playOnStart);
            ImGui::Checkbox("Spatial (3D)##as", &a.spatial);
            if (a.spatial) ImGui::DragFloat("Max distance##as", &a.maxDistance, 0.1f, 0.1f, 1000.f);
            char busBuf[64]; std::snprintf(busBuf, sizeof(busBuf), "%s", a.bus.c_str());
            if (ImGui::InputText("Bus##as", busBuf, sizeof(busBuf))) a.bus = busBuf;
            if (ImGui::Button("Preview##as")) audio::AudioEngine::instance().playOneShot(a.clipPath, a.volume);
            ImGui::SameLine();
            if (ImGui::Button("Remove##as")) entity->audioSource.reset();
        }
    }
    if (entity->audioListener.has_value()) {
        if (ImGui::CollapsingHeader("Audio Listener", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Active##al", &entity->audioListener->active);
            if (ImGui::Button("Remove##al")) entity->audioListener.reset();
        }
    }
    if (entity->uiRect.has_value()) {
        if (ImGui::CollapsingHeader("UI Rectangle", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& r = *entity->uiRect;
            const char* anchors[] = {
                "Top-Left","Top-Center","Top-Right",
                "Middle-Left","Center","Middle-Right",
                "Bottom-Left","Bottom-Center","Bottom-Right"};
            int an = static_cast<int>(r.anchor);
            if (ImGui::Combo("Anchor##uir", &an, anchors, IM_ARRAYSIZE(anchors))) {
                r.anchor = static_cast<scene::UIAnchor>(an);
            }
            ImGui::DragFloat("X##uir", &r.x, 1.f);
            ImGui::DragFloat("Y##uir", &r.y, 1.f);
            ImGui::DragFloat("Width##uir",  &r.width,  1.f, 1.f, 4096.f);
            ImGui::DragFloat("Height##uir", &r.height, 1.f, 1.f, 4096.f);
            ImGui::ColorEdit4("Color##uir", &r.r);
            if (ImGui::Button("Remove##uir")) entity->uiRect.reset();
        }
    }
    if (entity->uiText.has_value()) {
        if (ImGui::CollapsingHeader("UI Text", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& t = *entity->uiText;
            char buf[512]; std::snprintf(buf, sizeof(buf), "%s", t.text.c_str());
            if (ImGui::InputText("Text##uit", buf, sizeof(buf))) t.text = buf;
            ImGui::DragFloat("Font size##uit", &t.fontSize, 0.5f, 6.f, 256.f);
            ImGui::ColorEdit4("Color##uit", &t.r);
            if (ImGui::Button("Remove##uit")) entity->uiText.reset();
        }
    }
    if (entity->uiImage.has_value()) {
        if (ImGui::CollapsingHeader("UI Image", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& i = *entity->uiImage;
            char buf[256]; std::snprintf(buf, sizeof(buf), "%s", i.imagePath.c_str());
            if (ImGui::InputText("Path##uii", buf, sizeof(buf))) i.imagePath = buf;
            ImGui::ColorEdit4("Tint##uii", &i.r);
            if (ImGui::Button("Remove##uii")) entity->uiImage.reset();
        }
    }
    if (entity->uiButton.has_value()) {
        if (ImGui::CollapsingHeader("UI Button", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& b = *entity->uiButton;
            ImGui::ColorEdit4("Color##uib", &b.r);
            ImGui::ColorEdit4("Hover##uib", &b.hoverR);
            char scriptBuf[128]; std::snprintf(scriptBuf, sizeof(scriptBuf), "%s", b.onClickScript.c_str());
            if (ImGui::InputText("Script type##uib", scriptBuf, sizeof(scriptBuf))) b.onClickScript = scriptBuf;
            char methodBuf[64]; std::snprintf(methodBuf, sizeof(methodBuf), "%s", b.onClickMethod.c_str());
            if (ImGui::InputText("Method##uib", methodBuf, sizeof(methodBuf))) b.onClickMethod = methodBuf;
            if (ImGui::Button("Remove##uib")) entity->uiButton.reset();
        }
    }
    if (!entity->scripts.empty()) {
        if (ImGui::CollapsingHeader("Scripts", ImGuiTreeNodeFlags_DefaultOpen)) {
            int removeIdx = -1;
            for (size_t i = 0; i < entity->scripts.size(); ++i) {
                const auto& s = entity->scripts[i];
                ImGui::PushID(static_cast<int>(i));
                ImGui::BulletText("%s", s.typeName.c_str());
                ImGui::SameLine();
                if (ImGui::SmallButton("Remove")) removeIdx = static_cast<int>(i);
                ImGui::PopID();
            }
            if (removeIdx >= 0) entity->scripts.erase(entity->scripts.begin() + removeIdx);
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
        if (!entity->tint.has_value()) {
            if (ImGui::MenuItem("Color Tint")) entity->tint = scene::ColorTint{};
        }
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
        if (ImGui::BeginMenu("Audio")) {
            if (!entity->audioSource.has_value() && ImGui::MenuItem("Audio Source"))
                entity->audioSource = scene::AudioSourceComponent{};
            if (!entity->audioListener.has_value() && ImGui::MenuItem("Audio Listener"))
                entity->audioListener = scene::AudioListenerComponent{};
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("UI")) {
            if (!entity->uiRect.has_value()   && ImGui::MenuItem("Rectangle")) entity->uiRect = scene::UIRectComponent{};
            if (!entity->uiText.has_value()   && ImGui::MenuItem("Text"))      entity->uiText = scene::UITextComponent{};
            if (!entity->uiImage.has_value()  && ImGui::MenuItem("Image"))     entity->uiImage = scene::UIImageComponent{};
            if (!entity->uiButton.has_value() && ImGui::MenuItem("Button"))    entity->uiButton = scene::UIButtonComponent{};
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Script")) {
            const auto names = scripting::ScriptRegistry::instance().names();
            if (names.empty()) {
                ImGui::TextDisabled("(no scripts registered)");
            } else {
                for (const auto& n : names) {
                    if (ImGui::MenuItem(n.c_str())) {
                        scene::ScriptInstance si;
                        si.typeName = n;
                        si.instance = scripting::ScriptRegistry::instance().create(n);
                        if (si.instance) entity->scripts.push_back(std::move(si));
                    }
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

} // namespace hopf::editor
