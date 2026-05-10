#include "editor/panels/ViewportPanel.h"

#include "editor/AxisColors.h"
#include "scene/Scene.h"

#include <imgui.h>

#include <cmath>
#include <cstdio>

namespace hopf::editor {

namespace {

bool toggleButton(const char* label, const char* tooltip, bool active) {
    if (active) {
        const ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
        ImGui::PushStyleColor(ImGuiCol_Button, col);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
    }
    const bool clicked = ImGui::Button(label);
    if (active) ImGui::PopStyleColor(2);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip("%s", tooltip);
    }
    return clicked;
}

void verticalSep() {
    ImGui::SameLine();
    ImGui::TextUnformatted("|");
    ImGui::SameLine();
}

void planeSlider(const char* plane, float& val, float minV, float maxV) {
    textAxisColored(plane);
    ImGui::SameLine();
    char id[16];
    std::snprintf(id, sizeof(id), "##cam_%s", plane);
    ImGui::SetNextItemWidth(180.f);
    ImGui::SliderFloat(id, &val, minV, maxV);
}

} // namespace

ViewportPanel::ViewportPanel(int id) : m_id(id) {
    m_name = id == 0 ? "Viewport (4D)" : "Viewport #" + std::to_string(id);
}

void ViewportPanel::render(EditorContext& ctx) {
    if (!m_open) return;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (!ImGui::Begin(name(), &m_open)) {
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }
    ImGui::PopStyleVar();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,  ImVec2(4, 4));
    {
        if (toggleButton("Slice", "Slice mode\nIntersect the 4D scene with a hyperplane.\nThe 3D cross-section is rendered.",
                         m_camera.mode == render::ViewMode::Slice)) {
            m_camera.mode = render::ViewMode::Slice;
        }
        ImGui::SameLine();
        if (toggleButton("Project", "Projection mode\nProject the entire 4D scene down to 3D, then render.",
                         m_camera.mode == render::ViewMode::Projection)) {
            m_camera.mode = render::ViewMode::Projection;
        }

        verticalSep();

        if (toggleButton("Wire", "Simple wireframe", m_camera.style == render::DisplayStyle::SimpleWireframe)) {
            m_camera.style = render::DisplayStyle::SimpleWireframe;
        }
        ImGui::SameLine();
        if (toggleButton("Depth", "Depth wireframe (cool->warm gradient on w).",
                         m_camera.style == render::DisplayStyle::DepthWireframe)) {
            m_camera.style = render::DisplayStyle::DepthWireframe;
        }
        ImGui::SameLine();
        if (toggleButton("Unlit", "Solid faces, flat color.", m_camera.style == render::DisplayStyle::SolidUnlit)) {
            m_camera.style = render::DisplayStyle::SolidUnlit;
        }
        ImGui::SameLine();
        if (toggleButton("Lit", "Solid faces, lit by the scene's first directional light.",
                         m_camera.style == render::DisplayStyle::SolidLit)) {
            m_camera.style = render::DisplayStyle::SolidLit;
        }

        if (m_camera.mode == render::ViewMode::Slice) {
            verticalSep();
            constexpr char letters[4] = {'X', 'Y', 'Z', 'W'};
            constexpr ImVec4 axisCols[4] = {
                kAxisColorX, kAxisColorY, kAxisColorZ, kAxisColorW,
            };
            const char* tooltips[4] = {
                "Slice along X", "Slice along Y", "Slice along Z", "Slice along W",
            };
            for (int i = 0; i < 4; ++i) {
                if (i > 0) ImGui::SameLine();
                const bool active = (static_cast<int>(m_camera.sliceAxis) == i);
                const char buf[2] = {letters[i], '\0'};
                ImGui::PushStyleColor(ImGuiCol_Text, axisCols[i]);
                if (toggleButton(buf, tooltips[i], active)) {
                    m_camera.sliceAxis = static_cast<render::SliceAxis>(i);
                }
                ImGui::PopStyleColor();
            }
            ImGui::SameLine();
            constexpr const char* sliderLabels[4] = { "x slice", "y slice", "z slice", "w slice" };
            ImGui::SetNextItemWidth(180.f);
            ImGui::SliderFloat(sliderLabels[static_cast<int>(m_camera.sliceAxis)],
                               &m_camera.sliceVal, -1.5f, 1.5f, "%.3f");
        }

        verticalSep();

        if (ImGui::Button("Camera...")) ImGui::OpenPopup("##cameraPopup");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
            ImGui::SetTooltip("All viewport camera parameters: 3D orbit, 4D rotation, projection.");
        }
        if (ImGui::BeginPopup("##cameraPopup")) {
            if (ImGui::CollapsingHeader("Projection", ImGuiTreeNodeFlags_DefaultOpen)) {
                const bool isPersp = m_camera.projectionStyle == render::ProjectionStyle::Perspective;
                if (toggleButton("Persp",
                                 "Perspective in both 3D and 4D (foreshortening).",
                                 isPersp)) {
                    m_camera.projectionStyle = render::ProjectionStyle::Perspective;
                }
                ImGui::SameLine();
                if (toggleButton("Iso",
                                 "Isometric / orthographic in both 3D and 4D.",
                                 !isPersp)) {
                    m_camera.projectionStyle = render::ProjectionStyle::Isometric;
                }
                if (m_camera.projectionStyle == render::ProjectionStyle::Perspective) {
                    ImGui::SetNextItemWidth(160.f);
                    ImGui::DragFloat("focal4",  &m_camera.focal4,  0.05f, 0.5f, 20.f, "%.2f");
                    ImGui::SetNextItemWidth(160.f);
                    ImGui::DragFloat("wOffset", &m_camera.wOffset, 0.05f, -10.f, 10.f, "%.2f");
                }
                ImGui::SetNextItemWidth(160.f);
                ImGui::DragFloat("fov Y (deg)", &m_camera.fovYDeg, 0.5f, 5.f, 170.f);
                ImGui::SetNextItemWidth(160.f);
                ImGui::DragFloat("near", &m_camera.zNear, 0.01f, 0.001f, 100.f);
                ImGui::SetNextItemWidth(160.f);
                ImGui::DragFloat("far",  &m_camera.zFar,  1.0f,  1.f,    10000.f);
            }
            if (ImGui::CollapsingHeader("3D orbit", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::SetNextItemWidth(160.f);
                ImGui::DragFloat("yaw##cam",   &m_camera.yaw,   0.01f);
                ImGui::SetNextItemWidth(160.f);
                ImGui::DragFloat("pitch##cam", &m_camera.pitch, 0.01f);
                ImGui::SetNextItemWidth(160.f);
                ImGui::DragFloat("dist##cam",  &m_camera.distance, 0.05f, 0.05f, 100.f);
                ImGui::Text("Pivot:");
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, kAxisColorX);
                ImGui::SetNextItemWidth(60.f);
                ImGui::DragFloat("##pivX", &m_camera.pivotX, 0.05f);
                ImGui::PopStyleColor();
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, kAxisColorY);
                ImGui::SetNextItemWidth(60.f);
                ImGui::DragFloat("##pivY", &m_camera.pivotY, 0.05f);
                ImGui::PopStyleColor();
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, kAxisColorZ);
                ImGui::SetNextItemWidth(60.f);
                ImGui::DragFloat("##pivZ", &m_camera.pivotZ, 0.05f);
                ImGui::PopStyleColor();
                if (ImGui::Button("Recenter pivot")) {
                    m_camera.pivotX = m_camera.pivotY = m_camera.pivotZ = 0.f;
                }
            }
            if (ImGui::CollapsingHeader("4D rotation (rad, per plane)", ImGuiTreeNodeFlags_DefaultOpen)) {
                constexpr float kPi = 3.14159f;
                planeSlider("xy", m_camera.rotation4.xy, -kPi, kPi);
                planeSlider("xz", m_camera.rotation4.xz, -kPi, kPi);
                planeSlider("xw", m_camera.rotation4.xw, -kPi, kPi);
                planeSlider("yz", m_camera.rotation4.yz, -kPi, kPi);
                planeSlider("yw", m_camera.rotation4.yw, -kPi, kPi);
                planeSlider("zw", m_camera.rotation4.zw, -kPi, kPi);
                if (ImGui::Button("Reset 4D rotation")) m_camera.rotation4 = {};
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Grid...")) ImGui::OpenPopup("##gridPopup");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
            ImGui::SetTooltip("Toggle grid planes and their opacity.");
        }
        if (ImGui::BeginPopup("##gridPopup")) {
            ImGui::TextDisabled("Grids");
            ImGui::Separator();
            auto planeRow = [&](const char* label, bool& on) {
                textAxisColored(label);
                ImGui::SameLine(40.f);
                char id[16];
                std::snprintf(id, sizeof(id), "##grid_%s", label);
                ImGui::Checkbox(id, &on);
            };
            planeRow("xy", m_camera.grid.showXY);
            planeRow("xz", m_camera.grid.showXZ);
            planeRow("yz", m_camera.grid.showYZ);
            planeRow("xw", m_camera.grid.showXW);
            planeRow("yw", m_camera.grid.showYW);
            planeRow("zw", m_camera.grid.showZW);
            ImGui::Separator();
            ImGui::SetNextItemWidth(160.f);
            ImGui::SliderFloat("Opacity##grid", &m_camera.grid.opacity, 0.f, 1.f, "%.2f");
            ImGui::EndPopup();
        }
    }
    ImGui::PopStyleVar(2);

    ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.x > 1.f && avail.y > 1.f) {
        m_renderer.resize(static_cast<int>(avail.x), static_cast<int>(avail.y));
        if (ctx.scene) {
            m_renderer.render(*ctx.scene, m_camera);
        }

        ImGui::Image(static_cast<ImTextureID>(m_renderer.colorTexture()),
                     ImVec2(static_cast<float>(m_renderer.width()),
                            static_cast<float>(m_renderer.height())),
                     ImVec2(0, 1), ImVec2(1, 0));

        const ImVec2 imgMin = ImGui::GetItemRectMin();

        {
            const auto& labels = m_renderer.axisLabels();
            const char* axisChar[4] = {"x", "y", "z", "w"};
            const ImVec4 axisVec[4] = { kAxisColorX, kAxisColorY, kAxisColorZ, kAxisColorW };
            ImDrawList* dl = ImGui::GetWindowDrawList();
            for (int i = 0; i < 4; ++i) {
                if (!labels[i].visible) continue;
                const ImU32 col = ImGui::ColorConvertFloat4ToU32(axisVec[i]);
                const ImVec2 pos(imgMin.x + labels[i].u + 5.f, imgMin.y + labels[i].v - 6.f);
                dl->AddText(pos, col, axisChar[i]);
            }
        }

        const bool hovered = ImGui::IsItemHovered();
        const ImGuiIO& io = ImGui::GetIO();

        if (hovered && io.MouseWheel != 0.f) {
            m_camera.distance *= (io.MouseWheel > 0 ? 0.9f : 1.1f);
            m_camera.distance = std::max(0.05f, m_camera.distance);
        }

        const float cy = std::cos(m_camera.yaw);
        const float sy = std::sin(m_camera.yaw);
        const float cp = std::cos(m_camera.pitch);
        const float sp = std::sin(m_camera.pitch);

        const float right_x = cy;
        const float right_y = 0.f;
        const float right_z = -sy;

        const float up_x = -sy * sp;
        const float up_y =  cp;
        const float up_z = -cy * sp;

        if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
            const ImVec2 d = io.MouseDelta;
            m_camera.yaw   -= d.x * 0.005f;
            m_camera.pitch += d.y * 0.005f;
        }

        if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            const ImVec2 d = io.MouseDelta;
            const float speed = std::max(0.001f, m_camera.distance) * 0.0015f;
            m_camera.pivotX -= d.x * right_x * speed;
            m_camera.pivotY -= d.x * right_y * speed;
            m_camera.pivotZ -= d.x * right_z * speed;
            m_camera.pivotX += d.y * up_x * speed;
            m_camera.pivotY += d.y * up_y * speed;
            m_camera.pivotZ += d.y * up_z * speed;
        }

        // Click-to-pick: a left click without drag selects whatever entity is under the cursor.
        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            m_clickPending = true;
        }
        if (m_clickPending && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 5.f)) {
            m_clickPending = false;
        }
        if (m_clickPending && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            m_clickPending = false;
            if (hovered && ctx.scene) {
                const float u = io.MousePos.x - imgMin.x;
                const float v = io.MousePos.y - imgMin.y;
                ctx.selected = m_renderer.pickEntityAt(*ctx.scene, m_camera, u, v);
            }
        }
    }

    ImGui::End();
}

} // namespace hopf::editor
