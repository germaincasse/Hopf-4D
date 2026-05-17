#include "editor/panels/ViewportPanel.h"

#include "editor/AxisColors.h"
#include "scene/Scene.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

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

constexpr ImU32 kAxisColU32[4] = {
    IM_COL32(230, 80,  80,  255),  // X red
    IM_COL32(120, 220, 110, 255),  // Y green
    IM_COL32(90,  160, 240, 255),  // Z blue
    IM_COL32(240, 200, 90,  255),  // W yellow
};

// One color per rotation plane (xy, xz, xw, yz, yw, zw). Mixes of axis colors.
constexpr ImU32 kPlaneCol[6] = {
    IM_COL32(220, 180, 90,  255),  // xy
    IM_COL32(220, 100, 200, 255),  // xz
    IM_COL32(240, 150, 80,  255),  // xw
    IM_COL32(90,  220, 220, 255),  // yz
    IM_COL32(180, 230, 120, 255),  // yw
    IM_COL32(140, 170, 230, 255),  // zw
};
constexpr const char* kPlaneName[6] = {"xy", "xz", "xw", "yz", "yw", "zw"};

// Squared 2D distance helper.
inline float dist2(const ImVec2& a, const ImVec2& b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

// Squared perpendicular distance from point p to the segment [a, b]. Clamps the
// projection parameter to [0, 1] so endpoints stay at full hitbox radius.
inline float distToSegment2(const ImVec2& p, const ImVec2& a, const ImVec2& b) {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float len2 = dx * dx + dy * dy;
    if (len2 < 1e-6f) return dist2(p, a);
    float t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / len2;
    if (t < 0.f) t = 0.f; else if (t > 1.f) t = 1.f;
    const float qx = a.x + t * dx;
    const float qy = a.y + t * dy;
    const float ex = p.x - qx;
    const float ey = p.y - qy;
    return ex * ex + ey * ey;
}

// Draw a tool-bar icon (kind: 0=Hand, 1=Translate, 2=Rotate, 3=Scale) inside the last-laid
// button's rect. Called immediately after ImGui::Button.
void drawToolIcon(ImDrawList* dl, int kind, bool active) {
    const ImVec2 mn = ImGui::GetItemRectMin();
    const ImVec2 mx = ImGui::GetItemRectMax();
    const ImVec2 c { (mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f };
    const float r = (mx.y - mn.y) * 0.30f;
    const ImU32 col = active ? IM_COL32(255, 255, 255, 255) : IM_COL32(190, 200, 210, 255);
    if (kind == 0) {
        const float fw = r * 0.18f;
        for (int i = 0; i < 4; ++i) {
            const float x = c.x - r * 0.6f + i * fw * 2.f;
            dl->AddRectFilled(ImVec2(x - fw * 0.5f, c.y - r),
                              ImVec2(x + fw * 0.5f, c.y + r * 0.4f), col, 1.f);
        }
        dl->AddRectFilled(ImVec2(c.x + r * 0.5f, c.y - r * 0.2f),
                          ImVec2(c.x + r,        c.y + r * 0.6f), col, 1.f);
    } else if (kind == 1) {
        dl->AddLine(ImVec2(c.x - r, c.y), ImVec2(c.x + r, c.y), col, 1.8f);
        dl->AddLine(ImVec2(c.x, c.y - r), ImVec2(c.x, c.y + r), col, 1.8f);
        const float ah = r * 0.35f;
        dl->AddTriangleFilled(ImVec2(c.x + r, c.y), ImVec2(c.x + r - ah, c.y - ah * 0.6f), ImVec2(c.x + r - ah, c.y + ah * 0.6f), col);
        dl->AddTriangleFilled(ImVec2(c.x - r, c.y), ImVec2(c.x - r + ah, c.y - ah * 0.6f), ImVec2(c.x - r + ah, c.y + ah * 0.6f), col);
        dl->AddTriangleFilled(ImVec2(c.x, c.y + r), ImVec2(c.x - ah * 0.6f, c.y + r - ah), ImVec2(c.x + ah * 0.6f, c.y + r - ah), col);
        dl->AddTriangleFilled(ImVec2(c.x, c.y - r), ImVec2(c.x - ah * 0.6f, c.y - r + ah), ImVec2(c.x + ah * 0.6f, c.y - r + ah), col);
    } else if (kind == 2) {
        dl->AddCircle(c, r, col, 16, 1.8f);
        const float ax = c.x + r, ay = c.y;
        dl->AddTriangleFilled(ImVec2(ax + r * 0.25f, ay),
                              ImVec2(ax - r * 0.1f,  ay - r * 0.3f),
                              ImVec2(ax - r * 0.1f,  ay + r * 0.3f), col);
    } else {
        dl->AddLine(ImVec2(c.x - r * 0.7f, c.y + r * 0.7f),
                    ImVec2(c.x + r * 0.6f, c.y - r * 0.6f), col, 1.8f);
        dl->AddRectFilled(ImVec2(c.x + r * 0.4f, c.y - r),
                          ImVec2(c.x + r,        c.y - r * 0.4f), col, 1.f);
        dl->AddRectFilled(ImVec2(c.x - r,        c.y + r * 0.4f),
                          ImVec2(c.x - r * 0.4f, c.y + r),         col, 1.f);
    }
}

// Project a world point to screen, returning false if behind the near plane.
inline bool projectToScreen(const render::ViewportRenderer& r,
                            const render::Camera4D& cam,
                            const math::Vec4& world,
                            const ImVec2& imgMin,
                            ImVec2& outScreen)
{
    float u = 0.f, v = 0.f;
    if (!r.worldToScreen(cam, world, u, v)) return false;
    outScreen = ImVec2(imgMin.x + u, imgMin.y + v);
    return true;
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
        // Tool mode selector (Hand / Translate / Rotate / Scale).
        {
            const float btnH = ImGui::GetFrameHeight();
            const float btnW = btnH * 1.2f;
            ImDrawList* dl = ImGui::GetWindowDrawList();
            auto toolBtn = [&](const char* id, ToolMode mode, int kind, const char* tip) {
                const bool active = (ctx.toolMode == mode);
                if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                const bool clicked = ImGui::Button(id, ImVec2(btnW, 0));
                if (active) ImGui::PopStyleColor();
                drawToolIcon(dl, kind, active);
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) ImGui::SetTooltip("%s", tip);
                if (clicked) ctx.toolMode = mode;
            };
            toolBtn("##toolHand",      ToolMode::Hand,      0, "Hand (Q): select only");
            ImGui::SameLine();
            toolBtn("##toolTranslate", ToolMode::Translate, 1, "Translate (W): move along 4 axes");
            ImGui::SameLine();
            toolBtn("##toolRotate",    ToolMode::Rotate,    2, "Rotate (E): 6 planes in 4D");
            ImGui::SameLine();
            toolBtn("##toolScale",     ToolMode::Scale,     3, "Scale (R): resize per axis");
        }
        verticalSep();

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

        // Display-style icons (Wire / Depth / Unlit / Lit) with hover tooltips.
        {
            const float btnH = ImGui::GetFrameHeight();
            const float btnW = btnH * 1.2f;
            ImDrawList* dl = ImGui::GetWindowDrawList();

            auto styleBtn = [&](const char* id, render::DisplayStyle style,
                                int kind, const char* tip) {
                const bool active = (m_camera.style == style);
                if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                const bool clicked = ImGui::Button(id, ImVec2(btnW, 0));
                if (active) ImGui::PopStyleColor();

                const ImVec2 mn = ImGui::GetItemRectMin();
                const ImVec2 mx = ImGui::GetItemRectMax();
                const ImVec2 c { (mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f };
                const float  r = (mx.y - mn.y) * 0.30f;
                const ImU32 fg = active ? IM_COL32(255, 255, 255, 255) : IM_COL32(190, 200, 210, 255);

                if (kind == 0) {
                    // Wire: cube as two overlapping squares with connecting lines.
                    const ImVec2 a0(c.x - r, c.y - r * 0.6f);
                    const ImVec2 a1(c.x + r * 0.3f, c.y + r);
                    const ImVec2 b0(c.x - r * 0.3f, c.y - r);
                    const ImVec2 b1(c.x + r,        c.y + r * 0.6f);
                    dl->AddRect(a0, a1, fg, 0.f, 0, 1.4f);
                    dl->AddRect(b0, b1, fg, 0.f, 0, 1.4f);
                    dl->AddLine(a0, b0, fg, 1.0f);
                    dl->AddLine(ImVec2(a1.x, a0.y), ImVec2(b1.x, b0.y), fg, 1.0f);
                    dl->AddLine(a1, b1, fg, 1.0f);
                    dl->AddLine(ImVec2(a0.x, a1.y), ImVec2(b0.x, b1.y), fg, 1.0f);
                } else if (kind == 1) {
                    // Depth: same wireframe silhouette as Wire, but the back square
                    // is cool-toned and the front square is warm-toned -- conveys
                    // "wireframe coloured by depth".
                    const ImU32 cool = IM_COL32(80, 140, 255, 255);
                    const ImU32 warm = IM_COL32(255, 170, 80,  255);
                    const ImVec2 a0(c.x - r,         c.y - r * 0.6f);
                    const ImVec2 a1(c.x + r * 0.3f,  c.y + r);
                    const ImVec2 b0(c.x - r * 0.3f,  c.y - r);
                    const ImVec2 b1(c.x + r,         c.y + r * 0.6f);
                    dl->AddRect(a0, a1, cool, 0.f, 0, 1.4f);  // back face: cool
                    dl->AddRect(b0, b1, warm, 0.f, 0, 1.4f);  // front face: warm
                    // Connecting depth edges, also gradient-toned.
                    dl->AddLine(a0,                       b0,                       cool, 1.0f);
                    dl->AddLine(ImVec2(a1.x, a0.y),       ImVec2(b1.x, b0.y),       cool, 1.0f);
                    dl->AddLine(a1,                       b1,                       warm, 1.0f);
                    dl->AddLine(ImVec2(a0.x, a1.y),       ImVec2(b0.x, b1.y),       warm, 1.0f);
                } else if (kind == 2) {
                    // Unlit: filled square, flat color (no shading).
                    const ImVec2 p0(c.x - r * 0.9f, c.y - r * 0.9f);
                    const ImVec2 p1(c.x + r * 0.9f, c.y + r * 0.9f);
                    dl->AddRectFilled(p0, p1, fg, 1.f);
                } else if (kind == 3) {
                    // Lit: filled square + diagonal highlight + dark shadow band.
                    const ImVec2 p0(c.x - r * 0.9f, c.y - r * 0.9f);
                    const ImVec2 p1(c.x + r * 0.9f, c.y + r * 0.9f);
                    const ImU32 light = active ? IM_COL32(255, 255, 255, 255) : IM_COL32(230, 230, 230, 255);
                    const ImU32 dark  = IM_COL32(70, 70, 80, 255);
                    dl->AddRectFilledMultiColor(p0, p1, light, light, dark, dark);
                    dl->AddRect(p0, p1, IM_COL32(0, 0, 0, 180), 0.f, 0, 1.f);
                }

                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
                    ImGui::SetTooltip("%s", tip);
                }
                if (clicked) m_camera.style = style;
            };

            styleBtn("##styleWire",  render::DisplayStyle::SimpleWireframe, 0,
                     "Wire — simple wireframe");
            ImGui::SameLine();
            styleBtn("##styleDepth", render::DisplayStyle::DepthWireframe,  1,
                     "Depth — wireframe coloured by w (cool -> warm)");
            ImGui::SameLine();
            styleBtn("##styleUnlit", render::DisplayStyle::SolidUnlit,      2,
                     "Unlit — solid faces, flat color");
            ImGui::SameLine();
            styleBtn("##styleLit",   render::DisplayStyle::SolidLit,        3,
                     "Lit — solid faces, lit by directional lights");
        }

        if (m_camera.mode == render::ViewMode::Slice) {
            verticalSep();
            // Axis-aligned slice presets (most common case).
            constexpr char letters[4] = {'X', 'Y', 'Z', 'W'};
            constexpr ImVec4 axisCols[4] = {
                kAxisColorX, kAxisColorY, kAxisColorZ, kAxisColorW,
            };
            const char* tooltips[4] = {
                "Slice perpendicular to X",
                "Slice perpendicular to Y",
                "Slice perpendicular to Z",
                "Slice perpendicular to W",
            };
            auto axisAlignedTo = [&](int i) {
                const math::Vec4& a = m_camera.sliceAxis;
                const float coords[4] = {a.x, a.y, a.z, a.w};
                if (std::abs(coords[i]) < 0.999f) return false;
                for (int j = 0; j < 4; ++j) if (j != i && std::abs(coords[j]) > 1e-3f) return false;
                return true;
            };
            for (int i = 0; i < 4; ++i) {
                if (i > 0) ImGui::SameLine();
                const char buf[2] = {letters[i], '\0'};
                ImGui::PushStyleColor(ImGuiCol_Text, axisCols[i]);
                if (toggleButton(buf, tooltips[i], axisAlignedTo(i))) {
                    m_camera.sliceAxis = math::Vec4{};
                    (&m_camera.sliceAxis.x)[i] = 1.f;
                }
                ImGui::PopStyleColor();
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(180.f);
            ImGui::SliderFloat("slice", &m_camera.sliceVal, -1.5f, 1.5f, "%.3f");
        }

        verticalSep();

        // 2D / 3D / 4D viewing presets. They configure the camera so that the
        // scene is shown as if it were a 2D / 3D / 4D world (no other state
        // is changed apart from projection style, slice mode and orbit pose).
        {
            if (toggleButton("2D",
                             "View the scene as 2D: top-down isometric on the xy plane.\n"
                             "Slices through z = 0; w is collapsed.",
                             m_camera.mode == render::ViewMode::Slice
                              && std::abs(m_camera.sliceAxis.z) > 0.999f)) {
                m_camera.mode            = render::ViewMode::Slice;
                m_camera.sliceAxis       = {0, 0, 1, 0};
                m_camera.sliceVal        = 0.f;
                m_camera.projectionStyle = render::ProjectionStyle::Isometric;
                m_camera.yaw             = 0.f;
                m_camera.pitch           = -1.5707963f;
            }
            ImGui::SameLine();
            if (toggleButton("3D",
                             "View the scene as 3D: standard orbit, w sliced at 0.",
                             m_camera.mode == render::ViewMode::Slice
                              && std::abs(m_camera.sliceAxis.w) > 0.999f)) {
                m_camera.mode            = render::ViewMode::Slice;
                m_camera.sliceAxis       = {0, 0, 0, 1};
                m_camera.sliceVal        = 0.f;
                m_camera.projectionStyle = render::ProjectionStyle::Hybrid;
            }
            ImGui::SameLine();
            if (toggleButton("4D",
                             "View the scene as 4D: full projection mode (3D perspective by default).",
                             m_camera.mode == render::ViewMode::Projection)) {
                m_camera.mode            = render::ViewMode::Projection;
                m_camera.projectionStyle = render::ProjectionStyle::Hybrid;
            }
        }

        verticalSep();

        if (ImGui::Button("Camera...")) ImGui::OpenPopup("##cameraPopup");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
            ImGui::SetTooltip("All viewport camera parameters: 3D orbit, 4D rotation, projection.");
        }
        if (ImGui::BeginPopup("##cameraPopup")) {
            if (ImGui::CollapsingHeader("Projection", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (toggleButton("Persp",
                                 "Perspective in both 3D and 4D (foreshortening).",
                                 m_camera.projectionStyle == render::ProjectionStyle::Perspective)) {
                    m_camera.projectionStyle = render::ProjectionStyle::Perspective;
                }
                ImGui::SameLine();
                if (toggleButton("Hybrid",
                                 "3D perspective on screen, parallel 4D collapse.\n"
                                 "Useful when you want depth cues without 4D foreshortening.",
                                 m_camera.projectionStyle == render::ProjectionStyle::Hybrid)) {
                    m_camera.projectionStyle = render::ProjectionStyle::Hybrid;
                }
                ImGui::SameLine();
                if (toggleButton("Iso",
                                 "Isometric / orthographic in both 3D and 4D.",
                                 m_camera.projectionStyle == render::ProjectionStyle::Isometric)) {
                    m_camera.projectionStyle = render::ProjectionStyle::Isometric;
                }
                if (render::perspective4D(m_camera.projectionStyle)) {
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
            ImGui::SliderInt("Cells##grid", &m_camera.grid.cells, 2, 200);
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

        struct GizmoHit {
            scene::EntityId id;
            ImVec2          center;
            float           radius;
        };
        std::vector<GizmoHit> gizmoHits;
        ImDrawList* dlOverlay = ImGui::GetWindowDrawList();
        if (ctx.scene) {
            const float r = ImGui::GetFontSize() * 0.55f;
            for (const auto& e : ctx.scene->entities()) {
                const bool isLight  = e.light.has_value();
                const bool isCamera = e.camera2D.has_value()
                                   || e.camera3D.has_value()
                                   || e.camera4D.has_value();
                if (!isLight && !isCamera) continue;

                ImVec2 c;
                if (!projectToScreen(m_renderer, m_camera, e.transform.position, imgMin, c)) continue;
                gizmoHits.push_back({e.id, c, r * 1.2f});

                const bool sel = (ctx.selected == e.id);
                if (isLight) {
                    // Light tint defines the gizmo color.
                    auto clamp01 = [](float x) { return x < 0.f ? 0.f : (x > 1.f ? 1.f : x); };
                    const int lr = int(clamp01(e.light->r) * 255.f);
                    const int lg = int(clamp01(e.light->g) * 255.f);
                    const int lb = int(clamp01(e.light->b) * 255.f);
                    const ImU32 col = sel ? IM_COL32(std::min(255, lr + 40),
                                                     std::min(255, lg + 40),
                                                     std::min(255, lb + 40), 255)
                                          : IM_COL32(lr, lg, lb, 230);
                    dlOverlay->AddCircleFilled(c, r * 0.45f, col, 14);
                    for (int i = 0; i < 8; ++i) {
                        const float a = float(i) * 6.2831853f / 8.f;
                        const float ca = std::cos(a), sa = std::sin(a);
                        dlOverlay->AddLine(ImVec2(c.x + ca * r * 0.55f, c.y + sa * r * 0.55f),
                                           ImVec2(c.x + ca * r,         c.y + sa * r),
                                           col, 1.6f);
                    }
                    // Directional light direction arrow: rotation applied to canonical (0,-1,0,0).
                    const math::Vec4 dirCanon{0.f, -1.f, 0.f, 0.f};
                    const math::Vec4 dir4 = e.transform.rotation.toMatrix().transformDirection(dirCanon);
                    const float arrowLen = std::max(0.3f, m_camera.distance * 0.18f);
                    const math::Vec4 tipWorld{e.transform.position.x + dir4.x * arrowLen,
                                              e.transform.position.y + dir4.y * arrowLen,
                                              e.transform.position.z + dir4.z * arrowLen,
                                              e.transform.position.w + dir4.w * arrowLen};
                    ImVec2 tip;
                    if (projectToScreen(m_renderer, m_camera, tipWorld, imgMin, tip)) {
                        dlOverlay->AddLine(c, tip, col, 1.8f);
                        const float dxs = tip.x - c.x, dys = tip.y - c.y;
                        const float len = std::sqrt(dxs * dxs + dys * dys);
                        if (len > 1e-3f) {
                            const float ux = dxs / len, uy = dys / len;
                            const float px = -uy,        py =  ux;
                            const float ah = 7.f;
                            dlOverlay->AddTriangleFilled(
                                tip,
                                ImVec2(tip.x - ux * ah - px * ah * 0.45f, tip.y - uy * ah - py * ah * 0.45f),
                                ImVec2(tip.x - ux * ah + px * ah * 0.45f, tip.y - uy * ah + py * ah * 0.45f),
                                col);
                        }
                    }
                } else {
                    const ImU32 col = sel ? IM_COL32(230, 240, 255, 255)
                                          : IM_COL32(190, 200, 210, 230);
                    dlOverlay->AddRectFilled(ImVec2(c.x - r * 0.7f, c.y - r * 0.55f),
                                             ImVec2(c.x + r * 0.35f, c.y + r * 0.55f),
                                             col, 2.f);
                    dlOverlay->AddCircleFilled(ImVec2(c.x + r * 0.55f, c.y), r * 0.35f, col, 14);

                    // Unity-style frustum gizmo: a small rectangular pyramid with the apex
                    // at the camera entity and the base projected forward along its local
                    // -Z axis. Tells you at a glance which way the camera is looking.
                    const float forwardDist = std::max(0.6f, m_camera.distance * 0.20f);
                    float aspect = 1.55f, halfH = forwardDist * 0.35f;
                    if (e.camera2D.has_value()) {
                        halfH  = e.camera2D->orthoSize * 0.18f;
                    } else if (e.camera3D.has_value() && !e.camera3D->perspective) {
                        halfH  = e.camera3D->orthoHalfH * 0.18f;
                    }
                    const float halfW = halfH * aspect;
                    const math::Mat5 worldFromCam = ctx.scene->worldMatrix(e.id);
                    const math::Vec4 localCorners[4] = {
                        { +halfW, +halfH, -forwardDist, 0.f},
                        { -halfW, +halfH, -forwardDist, 0.f},
                        { -halfW, -halfH, -forwardDist, 0.f},
                        { +halfW, -halfH, -forwardDist, 0.f},
                    };
                    ImVec2 cornerScreen[4];
                    bool   cornerOk[4]{};
                    for (int i = 0; i < 4; ++i) {
                        const math::Vec4 wp = worldFromCam.transformPoint(localCorners[i]);
                        cornerOk[i] = projectToScreen(m_renderer, m_camera, wp, imgMin, cornerScreen[i]);
                    }
                    const ImU32 frustumCol = sel ? IM_COL32(220, 220, 230, 220)
                                                 : IM_COL32(170, 170, 180, 200);
                    for (int i = 0; i < 4; ++i) {
                        if (cornerOk[i]) dlOverlay->AddLine(c, cornerScreen[i], frustumCol, 1.0f);
                    }
                    for (int i = 0; i < 4; ++i) {
                        const int j = (i + 1) % 4;
                        if (cornerOk[i] && cornerOk[j]) {
                            dlOverlay->AddLine(cornerScreen[i], cornerScreen[j], frustumCol, 1.0f);
                        }
                    }
                }
            }
        }

        // Transform gizmos on the selected entity.
        // A handle is either a point (rotate disc, scale center) or a segment (whole arrow
        // shaft, so the user can grab the body of the axis, not only its tip).
        struct TransformHandle {
            int    handle;       // axis index (0..3) or plane index (0..5)
            ImVec2 center;       // tip (segments) or center (points)
            ImVec2 segStart;     // segments: arrow origin; points: same as center
            bool   segment;      // hit-test as segment if true, as disc otherwise
            float  radius;       // pickable radius in pixels (perpendicular for segments)
        };
        std::vector<TransformHandle> transformHits;
        scene::Entity* selectedEnt = (ctx.scene && ctx.selected != 0)
                                        ? ctx.scene->findEntity(ctx.selected)
                                        : nullptr;
        if (selectedEnt && ctx.toolMode != ToolMode::Hand) {
            ImVec2 originScreen;
            // World position of the selected entity (walks parents).
            const math::Vec4 selWorldPos =
                ctx.scene->worldMatrix(selectedEnt->id).transformPoint({0.f, 0.f, 0.f, 0.f});
            if (projectToScreen(m_renderer, m_camera, selWorldPos, imgMin, originScreen)) {
                const float gizmoLen = std::max(0.3f, m_camera.distance * 0.18f);
                const ImVec2 mp      = ImGui::GetIO().MousePos;

                auto worldDir = [&](int axis, float length) {
                    math::Vec4 p = selWorldPos;
                    p[axis] += length;
                    return p;
                };

                // Brighten a base axis color when the cursor is over its handle.
                auto highlight = [](ImU32 c) {
                    const int r = std::min(255, int((c >> IM_COL32_R_SHIFT) & 0xFF) + 50);
                    const int g = std::min(255, int((c >> IM_COL32_G_SHIFT) & 0xFF) + 50);
                    const int b = std::min(255, int((c >> IM_COL32_B_SHIFT) & 0xFF) + 50);
                    return IM_COL32(r, g, b, 255);
                };

                if (ctx.toolMode == ToolMode::Translate || ctx.toolMode == ToolMode::Scale) {
                    const bool isScale = (ctx.toolMode == ToolMode::Scale);
                    const float segHitR = 9.f; // perpendicular pixels picking radius for arrow shafts
                    for (int axis = 0; axis < 4; ++axis) {
                        ImVec2 tip;
                        if (!projectToScreen(m_renderer, m_camera,
                                             worldDir(axis, gizmoLen), imgMin, tip)) continue;
                        const bool hov = (distToSegment2(mp, originScreen, tip) <= segHitR * segHitR)
                                      || (m_drag.active && m_drag.handle == axis && m_drag.mode == ctx.toolMode);
                        const ImU32 col = hov ? highlight(kAxisColU32[axis]) : kAxisColU32[axis];
                        dlOverlay->AddLine(originScreen, tip, col, hov ? 3.5f : 2.f);

                        if (isScale) {
                            const float s = hov ? 7.f : 5.f;
                            dlOverlay->AddRectFilled(ImVec2(tip.x - s, tip.y - s),
                                                     ImVec2(tip.x + s, tip.y + s), col, 1.5f);
                        } else {
                            const float dxs = tip.x - originScreen.x, dys = tip.y - originScreen.y;
                            const float len = std::sqrt(dxs * dxs + dys * dys);
                            if (len > 1e-3f) {
                                const float ux = dxs / len, uy = dys / len;
                                const float px = -uy,        py =  ux;
                                const float ah = hov ? 9.f : 7.f;
                                dlOverlay->AddTriangleFilled(
                                    tip,
                                    ImVec2(tip.x - ux * ah - px * ah * 0.5f, tip.y - uy * ah - py * ah * 0.5f),
                                    ImVec2(tip.x - ux * ah + px * ah * 0.5f, tip.y - uy * ah + py * ah * 0.5f),
                                    col);
                            }
                        }
                        transformHits.push_back({axis, tip, originScreen, true, segHitR});
                    }
                    if (isScale) {
                        // Center handle for uniform scale.
                        const float ptR = 14.f;
                        const bool hov = (dist2(mp, originScreen) <= ptR * ptR)
                                      || (m_drag.active && m_drag.handle == 4);
                        const ImU32 col = hov ? IM_COL32(255, 255, 255, 255) : IM_COL32(220, 220, 230, 255);
                        const float s = hov ? 7.f : 5.f;
                        dlOverlay->AddRectFilled(ImVec2(originScreen.x - s, originScreen.y - s),
                                                 ImVec2(originScreen.x + s, originScreen.y + s),
                                                 col, 1.5f);
                        transformHits.push_back({4, originScreen, originScreen, false, ptR});
                    }
                } else if (ctx.toolMode == ToolMode::Rotate) {
                    // 6 colored discs arranged in a 2x3 grid above the entity.
                    const float spacing = 26.f;
                    const float baseX = originScreen.x - spacing * 1.0f;
                    const float baseY = originScreen.y - 90.f;
                    const float discR = 11.f;
                    for (int p = 0; p < 6; ++p) {
                        const int col = p % 3, row = p / 3;
                        const ImVec2 c{baseX + col * spacing, baseY + row * spacing};
                        const bool hov = (dist2(mp, c) <= (discR + 4.f) * (discR + 4.f))
                                      || (m_drag.active && m_drag.mode == ToolMode::Rotate && m_drag.handle == p);
                        const ImU32 fill = hov ? highlight(kPlaneCol[p]) : kPlaneCol[p];
                        dlOverlay->AddCircleFilled(c, hov ? discR + 1.f : discR, fill, 18);
                        dlOverlay->AddCircle(c, hov ? discR + 1.f : discR, IM_COL32(0, 0, 0, 180), 18, 1.f);
                        dlOverlay->AddText(ImVec2(c.x - 8.f, c.y - 7.f),
                                           IM_COL32(20, 20, 25, 255), kPlaneName[p]);
                        transformHits.push_back({p, c, c, false, discR + 4.f});
                    }
                    // Faint cross on the entity to visually anchor the gizmo.
                    dlOverlay->AddLine(ImVec2(originScreen.x - 8, originScreen.y),
                                       ImVec2(originScreen.x + 8, originScreen.y),
                                       IM_COL32(255, 255, 255, 120), 1.f);
                    dlOverlay->AddLine(ImVec2(originScreen.x, originScreen.y - 8),
                                       ImVec2(originScreen.x, originScreen.y + 8),
                                       IM_COL32(255, 255, 255, 120), 1.f);
                }
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

        // Try to pick up a transform-gizmo handle on left-click.
        if (hovered && !m_drag.active && selectedEnt
            && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            int picked = -1;
            float bestD2 = 1e9f;
            for (const auto& h : transformHits) {
                const float d2 = h.segment
                                ? distToSegment2(io.MousePos, h.segStart, h.center)
                                : dist2(io.MousePos, h.center);
                if (d2 <= h.radius * h.radius && d2 < bestD2) { bestD2 = d2; picked = h.handle; }
            }
            if (picked >= 0) {
                ctx.pushUndo();
                m_drag.active          = true;
                m_drag.entityId        = ctx.selected;
                m_drag.mode            = ctx.toolMode;
                m_drag.handle          = picked;
                m_drag.mouseStart      = io.MousePos;
                m_drag.initialPos      = selectedEnt->transform.position;
                m_drag.initialWorldPos = ctx.scene->worldMatrix(selectedEnt->id)
                                            .transformPoint({0.f, 0.f, 0.f, 0.f});
                m_drag.initialScale    = selectedEnt->transform.scale;
                m_drag.initialRot      = selectedEnt->transform.rotation;
                m_clickPending         = false;
            }
        }

        // While dragging, update the entity's transform based on the mouse delta.
        if (m_drag.active && ctx.scene) {
            auto* dragEnt = ctx.scene->findEntity(m_drag.entityId);
            if (!dragEnt) {
                m_drag.active = false;
            } else {
                const ImVec2 mouseDelta{io.MousePos.x - m_drag.mouseStart.x,
                                        io.MousePos.y - m_drag.mouseStart.y};
                if (m_drag.mode == ToolMode::Translate) {
                    if (m_drag.handle >= 0 && m_drag.handle < 4) {
                        const int axis = m_drag.handle;
                        // Recompute the axis screen direction at the *initial* position to avoid drift.
                        ImVec2 o, t;
                        if (projectToScreen(m_renderer, m_camera, m_drag.initialWorldPos, imgMin, o)) {
                            math::Vec4 tipW = m_drag.initialWorldPos; tipW[axis] += 1.f;
                            if (projectToScreen(m_renderer, m_camera, tipW, imgMin, t)) {
                                const float dx = t.x - o.x, dy = t.y - o.y;
                                const float lenSq = dx * dx + dy * dy;
                                if (lenSq > 1e-4f) {
                                    const float amount = (mouseDelta.x * dx + mouseDelta.y * dy) / lenSq;
                                    dragEnt->transform.position = m_drag.initialPos;
                                    dragEnt->transform.position[axis] += amount;
                                }
                            }
                        }
                    }
                } else if (m_drag.mode == ToolMode::Scale) {
                    if (m_drag.handle >= 0 && m_drag.handle < 4) {
                        const int axis = m_drag.handle;
                        ImVec2 o, t;
                        if (projectToScreen(m_renderer, m_camera, m_drag.initialWorldPos, imgMin, o)) {
                            math::Vec4 tipW = m_drag.initialWorldPos; tipW[axis] += 1.f;
                            if (projectToScreen(m_renderer, m_camera, tipW, imgMin, t)) {
                                const float dx = t.x - o.x, dy = t.y - o.y;
                                const float lenSq = dx * dx + dy * dy;
                                if (lenSq > 1e-4f) {
                                    const float amount = (mouseDelta.x * dx + mouseDelta.y * dy) / lenSq;
                                    dragEnt->transform.scale = m_drag.initialScale;
                                    dragEnt->transform.scale[axis] = std::max(0.01f,
                                        m_drag.initialScale[axis] * std::exp(amount * 0.5f));
                                }
                            }
                        }
                    } else if (m_drag.handle == 4) {
                        // Uniform scale: drag right grows, drag left shrinks.
                        const float amount = (mouseDelta.x - mouseDelta.y) * 0.01f;
                        const float k = std::exp(amount);
                        dragEnt->transform.scale.x = std::max(0.01f, m_drag.initialScale.x * k);
                        dragEnt->transform.scale.y = std::max(0.01f, m_drag.initialScale.y * k);
                        dragEnt->transform.scale.z = std::max(0.01f, m_drag.initialScale.z * k);
                        dragEnt->transform.scale.w = std::max(0.01f, m_drag.initialScale.w * k);
                    }
                } else if (m_drag.mode == ToolMode::Rotate) {
                    if (m_drag.handle >= 0 && m_drag.handle < 6) {
                        // Drag horizontally = rotate around this plane. 1 px = ~0.5 deg.
                        const float amount = mouseDelta.x * 0.01f;
                        dragEnt->transform.rotation = m_drag.initialRot;
                        switch (m_drag.handle) {
                            case 0: dragEnt->transform.rotation.xy += amount; break;
                            case 1: dragEnt->transform.rotation.xz += amount; break;
                            case 2: dragEnt->transform.rotation.xw += amount; break;
                            case 3: dragEnt->transform.rotation.yz += amount; break;
                            case 4: dragEnt->transform.rotation.yw += amount; break;
                            case 5: dragEnt->transform.rotation.zw += amount; break;
                        }
                    }
                }
            }
        }
        if (m_drag.active && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            m_drag.active = false;
        }

        // Camera pan on left drag (only when no gizmo drag is in progress).
        if (hovered && !m_drag.active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            const ImVec2 d = io.MouseDelta;
            const float speed = std::max(0.001f, m_camera.distance) * 0.0015f;
            m_camera.pivotX -= d.x * right_x * speed;
            m_camera.pivotY -= d.x * right_y * speed;
            m_camera.pivotZ -= d.x * right_z * speed;
            m_camera.pivotX += d.y * up_x * speed;
            m_camera.pivotY += d.y * up_y * speed;
            m_camera.pivotZ += d.y * up_z * speed;
        }

        // Click-to-pick: a left click without drag selects whatever entity is under the cursor,
        // unless the click already started a gizmo drag.
        if (hovered && !m_drag.active && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            m_clickPending = true;
        }
        if (m_clickPending && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 5.f)) {
            m_clickPending = false;
        }
        if (m_clickPending && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            m_clickPending = false;
            if (hovered && ctx.scene) {
                scene::EntityId gizmoId = 0;
                for (const auto& g : gizmoHits) {
                    if (dist2(io.MousePos, g.center) <= g.radius * g.radius) {
                        gizmoId = g.id;
                        break;
                    }
                }
                if (gizmoId != 0) {
                    ctx.selected = gizmoId;
                } else {
                    const float u = io.MousePos.x - imgMin.x;
                    const float v = io.MousePos.y - imgMin.y;
                    ctx.selected = m_renderer.pickEntityAt(*ctx.scene, m_camera, u, v);
                }
            }
        }

        // F: focus camera on the selected entity. Repeated presses toggle a close /
        // far distance. The camera lerps toward the target each frame for smoothness.
        if (hovered && !io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_F, false)
            && selectedEnt && ctx.scene)
        {
            const math::Vec4 wp = ctx.scene->worldMatrix(selectedEnt->id)
                                       .transformPoint({0.f, 0.f, 0.f, 0.f});
            m_focusActive   = true;
            m_focusFar      = !m_focusFar;
            m_focusPivotX   = wp.x;
            m_focusPivotY   = wp.y;
            m_focusPivotZ   = wp.z;
            m_focusDistance = m_focusFar ? 12.f : 4.f;
        }
        if (m_focusActive) {
            const float dt = ImGui::GetIO().DeltaTime;
            const float k  = 1.f - std::exp(-8.f * dt);
            m_camera.pivotX   += (m_focusPivotX   - m_camera.pivotX)   * k;
            m_camera.pivotY   += (m_focusPivotY   - m_camera.pivotY)   * k;
            m_camera.pivotZ   += (m_focusPivotZ   - m_camera.pivotZ)   * k;
            m_camera.distance += (m_focusDistance - m_camera.distance) * k;
            const float dx = m_focusPivotX - m_camera.pivotX;
            const float dy = m_focusPivotY - m_camera.pivotY;
            const float dz = m_focusPivotZ - m_camera.pivotZ;
            const float dd = m_focusDistance - m_camera.distance;
            if (dx*dx + dy*dy + dz*dz < 1e-4f && std::abs(dd) < 1e-3f) {
                m_focusActive = false;
            }
        }
    }

    ImGui::End();
}

} // namespace hopf::editor
