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
        if (toggleButton("Slice", "Slice mode\nIntersect the 4D scene with the hyperplane w = sliceW.\nThe 3D cross-section is rendered.",
                         m_camera.mode == render::ViewMode::Slice)) {
            m_camera.mode = render::ViewMode::Slice;
        }
        ImGui::SameLine();
        if (toggleButton("Project", "Projection mode\nProject the entire 4D scene down to 3D, then render.\n(perspective along the w axis)",
                         m_camera.mode == render::ViewMode::Projection)) {
            m_camera.mode = render::ViewMode::Projection;
        }

        verticalSep();

        if (toggleButton("Wire", "Simple wireframe\nEdges only, uniform color.",
                         m_camera.style == render::DisplayStyle::SimpleWireframe)) {
            m_camera.style = render::DisplayStyle::SimpleWireframe;
        }
        ImGui::SameLine();
        if (toggleButton("Depth", "Depth wireframe\nEdges colored by their w coordinate (cool->warm gradient).\nMost informative in projection mode.",
                         m_camera.style == render::DisplayStyle::DepthWireframe)) {
            m_camera.style = render::DisplayStyle::DepthWireframe;
        }
        ImGui::SameLine();
        if (toggleButton("Unlit", "Unlit\nOpaque filled faces with a flat color.",
                         m_camera.style == render::DisplayStyle::SolidUnlit)) {
            m_camera.style = render::DisplayStyle::SolidUnlit;
        }
        ImGui::SameLine();
        if (toggleButton("Lit", "Lit\nOpaque filled faces with derivative-normal screen-space lighting.",
                         m_camera.style == render::DisplayStyle::SolidLit)) {
            m_camera.style = render::DisplayStyle::SolidLit;
        }

        verticalSep();

        if (m_camera.mode == render::ViewMode::Slice) {
            ImGui::SetNextItemWidth(180.f);
            ImGui::SliderFloat("w slice", &m_camera.sliceW, -1.5f, 1.5f, "%.3f");
        } else {
            ImGui::Checkbox("Persp4", &m_camera.perspective4);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80.f);
            ImGui::DragFloat("focal", &m_camera.focal4, 0.05f, 0.5f, 20.f, "%.2f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80.f);
            ImGui::DragFloat("wOff", &m_camera.wOffset, 0.05f, -10.f, 10.f, "%.2f");
        }

        verticalSep();

        if (ImGui::Button("Cam4D...")) ImGui::OpenPopup("##cam4dPopup");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
            ImGui::SetTooltip("Camera 4D rotation (six rotation planes).");
        }
        if (ImGui::BeginPopup("##cam4dPopup")) {
            ImGui::TextDisabled("Camera 4D rotation (rad)");
            ImGui::Separator();
            constexpr float kPi = 3.14159f;
            planeSlider("xy", m_camera.rotation4.xy, -kPi, kPi);
            planeSlider("xz", m_camera.rotation4.xz, -kPi, kPi);
            planeSlider("xw", m_camera.rotation4.xw, -kPi, kPi);
            planeSlider("yz", m_camera.rotation4.yz, -kPi, kPi);
            planeSlider("yw", m_camera.rotation4.yw, -kPi, kPi);
            planeSlider("zw", m_camera.rotation4.zw, -kPi, kPi);
            if (ImGui::Button("Reset")) m_camera.rotation4 = {};
            ImGui::EndPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Recenter")) {
            m_camera.pivotX = m_camera.pivotY = m_camera.pivotZ = 0.f;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
            ImGui::SetTooltip("Reset orbit pivot to world origin.");
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

        {
            const ImVec2 imgMin = ImGui::GetItemRectMin();
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
    }

    ImGui::End();
}

} // namespace hopf::editor
