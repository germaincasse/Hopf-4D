#include "editor/Editor.h"

#include "editor/panels/ConsolePanel.h"
#include "editor/panels/FilesPanel.h"
#include "editor/panels/GameViewPanel.h"
#include "editor/panels/HierarchyPanel.h"
#include "editor/panels/InspectorPanel.h"
#include "editor/panels/ViewportPanel.h"

#include "scene/Scene.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <algorithm>

namespace hopf::editor {

Editor::~Editor() {
    if (m_initialized) shutdown();
}

void Editor::init(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = "imgui.ini";

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding     = ImVec2(8, 8);
    style.FramePadding      = ImVec2(8, 4);
    style.CellPadding       = ImVec2(6, 4);
    style.ItemSpacing       = ImVec2(8, 4);
    style.ItemInnerSpacing  = ImVec2(6, 4);
    style.IndentSpacing     = 18.f;
    style.ScrollbarSize     = 12.f;
    style.GrabMinSize       = 12.f;
    style.WindowRounding    = 4.f;
    style.ChildRounding     = 4.f;
    style.FrameRounding     = 4.f;
    style.PopupRounding     = 4.f;
    style.GrabRounding      = 3.f;
    style.TabRounding       = 4.f;
    style.ScrollbarRounding = 6.f;
    style.WindowBorderSize  = 1.f;
    style.FrameBorderSize   = 0.f;
    style.PopupBorderSize   = 1.f;
    style.WindowTitleAlign  = ImVec2(0.0f, 0.5f);

    ImVec4* c = style.Colors;
    const ImVec4 bg      = ImVec4(0.145f, 0.149f, 0.165f, 1.00f);
    const ImVec4 bgDark  = ImVec4(0.110f, 0.114f, 0.125f, 1.00f);
    const ImVec4 frame   = ImVec4(0.180f, 0.188f, 0.208f, 1.00f);
    const ImVec4 frameH  = ImVec4(0.235f, 0.243f, 0.275f, 1.00f);
    const ImVec4 frameA  = ImVec4(0.290f, 0.310f, 0.355f, 1.00f);
    const ImVec4 border  = ImVec4(0.063f, 0.071f, 0.090f, 1.00f);
    const ImVec4 accent  = ImVec4(0.388f, 0.557f, 0.792f, 1.00f);
    const ImVec4 accentH = ImVec4(0.486f, 0.659f, 0.886f, 1.00f);
    const ImVec4 text    = ImVec4(0.847f, 0.851f, 0.867f, 1.00f);
    const ImVec4 textDim = ImVec4(0.514f, 0.529f, 0.553f, 1.00f);

    c[ImGuiCol_Text]                  = text;
    c[ImGuiCol_TextDisabled]          = textDim;
    c[ImGuiCol_WindowBg]              = bg;
    c[ImGuiCol_ChildBg]               = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_PopupBg]               = bgDark;
    c[ImGuiCol_Border]                = border;
    c[ImGuiCol_BorderShadow]          = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_FrameBg]               = frame;
    c[ImGuiCol_FrameBgHovered]        = frameH;
    c[ImGuiCol_FrameBgActive]         = frameA;
    c[ImGuiCol_TitleBg]               = bgDark;
    c[ImGuiCol_TitleBgActive]         = ImVec4(0.165f, 0.173f, 0.196f, 1.f);
    c[ImGuiCol_TitleBgCollapsed]      = bgDark;
    c[ImGuiCol_MenuBarBg]             = bgDark;
    c[ImGuiCol_ScrollbarBg]           = bg;
    c[ImGuiCol_ScrollbarGrab]         = frame;
    c[ImGuiCol_ScrollbarGrabHovered]  = frameH;
    c[ImGuiCol_ScrollbarGrabActive]   = frameA;
    c[ImGuiCol_CheckMark]             = accent;
    c[ImGuiCol_SliderGrab]            = accent;
    c[ImGuiCol_SliderGrabActive]      = accentH;
    c[ImGuiCol_Button]                = frame;
    c[ImGuiCol_ButtonHovered]         = frameH;
    c[ImGuiCol_ButtonActive]          = ImVec4(accent.x * 0.85f, accent.y * 0.85f, accent.z * 0.85f, 1.f);
    c[ImGuiCol_Header]                = ImVec4(0.220f, 0.275f, 0.345f, 1.f);
    c[ImGuiCol_HeaderHovered]         = ImVec4(0.275f, 0.345f, 0.420f, 1.f);
    c[ImGuiCol_HeaderActive]          = ImVec4(0.330f, 0.420f, 0.510f, 1.f);
    c[ImGuiCol_Separator]             = border;
    c[ImGuiCol_SeparatorHovered]      = frameH;
    c[ImGuiCol_SeparatorActive]       = accent;
    c[ImGuiCol_ResizeGrip]            = ImVec4(0.30f, 0.30f, 0.32f, 0.40f);
    c[ImGuiCol_ResizeGripHovered]     = ImVec4(accent.x, accent.y, accent.z, 0.65f);
    c[ImGuiCol_ResizeGripActive]      = accent;
    c[ImGuiCol_Tab]                   = bgDark;
    c[ImGuiCol_TabHovered]            = frameH;
    c[ImGuiCol_TabActive]             = frame;
    c[ImGuiCol_TabUnfocused]          = bgDark;
    c[ImGuiCol_TabUnfocusedActive]    = frame;
    c[ImGuiCol_DockingPreview]        = ImVec4(accent.x, accent.y, accent.z, 0.55f);
    c[ImGuiCol_DockingEmptyBg]        = bgDark;
    c[ImGuiCol_TextSelectedBg]        = ImVec4(accent.x, accent.y, accent.z, 0.40f);
    c[ImGuiCol_NavHighlight]          = accent;

    m_baseStyle = style;
    applyUiScale();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    m_panels.emplace_back(std::make_unique<ViewportPanel>(0));
    m_panels.emplace_back(std::make_unique<GameViewPanel>());
    m_panels.emplace_back(std::make_unique<HierarchyPanel>());
    m_panels.emplace_back(std::make_unique<InspectorPanel>());
    m_panels.emplace_back(std::make_unique<ConsolePanel>());
    m_panels.emplace_back(std::make_unique<FilesPanel>());

    m_initialized = true;
}

void Editor::shutdown() {
    if (!m_initialized) return;
    m_panels.clear();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}

void Editor::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Editor::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Editor::buildDefaultLayout(unsigned int dockspaceId) {
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->Size);

    ImGuiID center    = dockspaceId;
    ImGuiID right     = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.22f, nullptr, &center);
    ImGuiID left      = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left,  0.20f, nullptr, &center);
    ImGuiID bottom    = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down,  0.25f, nullptr, &center);
    // Split the remaining center horizontally so Viewport sits left and Game sits right.
    ImGuiID gameRight = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.45f, nullptr, &center);

    ImGui::DockBuilderDockWindow("Viewport (4D)", center);
    ImGui::DockBuilderDockWindow("Game",          gameRight);
    ImGui::DockBuilderDockWindow("Hierarchy",     left);
    ImGui::DockBuilderDockWindow("Inspector",     right);
    ImGui::DockBuilderDockWindow("Console",       bottom);
    ImGui::DockBuilderDockWindow("Files",         bottom);

    ImGui::DockBuilderFinish(dockspaceId);
}

void Editor::renderUI(scene::Scene& scene) {
    m_ctx.scene = &scene;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);

    ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize   | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoDocking  | ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::Begin("##DockHost", nullptr, hostFlags);
    ImGui::PopStyleVar(3);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Window")) {
            for (auto& p : m_panels) {
                bool open = p->isOpen();
                if (ImGui::MenuItem(p->name(), nullptr, &open)) {
                    p->setOpen(open);
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Add Viewport")) {
                m_panels.emplace_back(std::make_unique<ViewportPanel>(m_nextViewportId++));
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Reset layout")) m_layoutBuilt = false;
            ImGui::EndMenu();
        }

        // Play/Stop toggle + Pause, centered in the menu bar.
        {
            const float btnW       = ImGui::GetFrameHeight() * 1.4f;
            const float spacing    = ImGui::GetStyle().ItemSpacing.x;
            const float totalW     = btnW * 2.f + spacing;
            const float available  = ImGui::GetContentRegionAvail().x;
            const float offset     = std::max(0.f, (available - totalW) * 0.5f);
            if (offset > 0.f) ImGui::Dummy(ImVec2(offset, 0.f));

            auto drawIcon = [&](int kind, bool dim) {
                const ImVec2 mn = ImGui::GetItemRectMin();
                const ImVec2 mx = ImGui::GetItemRectMax();
                const ImVec2 c { (mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f };
                const float r = (mx.y - mn.y) * 0.28f;
                ImDrawList* dl = ImGui::GetWindowDrawList();
                const int a = dim ? 110 : 255;
                if (kind == 0) {
                    const ImU32 col = IM_COL32(110, 220, 130, a);
                    dl->AddTriangleFilled(ImVec2(c.x - r * 0.6f, c.y - r),
                                          ImVec2(c.x - r * 0.6f, c.y + r),
                                          ImVec2(c.x + r,        c.y),
                                          col);
                } else if (kind == 1) {
                    const ImU32 col = IM_COL32(245, 200, 80, a);
                    const float w2 = r * 0.35f;
                    dl->AddRectFilled(ImVec2(c.x - r,         c.y - r),
                                      ImVec2(c.x - r + w2,    c.y + r), col);
                    dl->AddRectFilled(ImVec2(c.x + r - w2,    c.y - r),
                                      ImVec2(c.x + r,         c.y + r), col);
                } else {
                    const ImU32 col = IM_COL32(230, 110, 110, a);
                    dl->AddRectFilled(ImVec2(c.x - r, c.y - r), ImVec2(c.x + r, c.y + r), col);
                }
            };

            const bool paused  = (m_ctx.playState == PlayState::Paused);
            const bool stopped = (m_ctx.playState == PlayState::Stopped);
            const bool running = !stopped;

            if (running) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            const bool clickedPlayStop = ImGui::Button("##playstop", ImVec2(btnW, 0));
            if (running) ImGui::PopStyleColor();
            drawIcon(running ? 2 : 0, false);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
                ImGui::SetTooltip("%s", running ? "Stop and reset the simulation" : "Play the simulation");
            }
            if (clickedPlayStop) {
                if (stopped) {
                    snapshotScene(scene);
                    m_ctx.playState = PlayState::Playing;
                } else {
                    restoreScene(scene);
                    m_ctx.playState = PlayState::Stopped;
                }
            }

            ImGui::SameLine();

            if (paused) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            ImGui::BeginDisabled(stopped);
            const bool clickedPause = ImGui::Button("##pause", ImVec2(btnW, 0));
            ImGui::EndDisabled();
            if (paused) ImGui::PopStyleColor();
            drawIcon(1, stopped);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
                ImGui::SetTooltip("%s", paused ? "Resume" : "Pause");
            }
            if (clickedPause) {
                m_ctx.playState = paused ? PlayState::Playing : PlayState::Paused;
            }
        }

        ImGui::EndMenuBar();
    }

    const ImGuiID dockspaceId = ImGui::GetID("hopf-Dockspace");
    if (!m_layoutBuilt) {
        buildDefaultLayout(dockspaceId);
        m_layoutBuilt = true;
    }
    ImGui::DockSpace(dockspaceId, ImVec2(0, 0), ImGuiDockNodeFlags_None);
    ImGui::End();

    for (auto& p : m_panels) {
        if (p->isOpen()) p->render(m_ctx);
    }

    processShortcuts();
}

void Editor::snapshotScene(scene::Scene& scene) {
    m_sceneSnapshot = scene.entities();
}

void Editor::restoreScene(scene::Scene& scene) {
    if (m_sceneSnapshot.empty()) return;
    scene.entities() = m_sceneSnapshot;
    m_sceneSnapshot.clear();
}

void Editor::applyUiScale() {
    ImGuiStyle& s = ImGui::GetStyle();
    s = m_baseStyle;
    s.ScaleAllSizes(m_uiScale);
    ImGui::GetIO().FontGlobalScale = m_uiScale;
}

void Editor::processShortcuts() {
    const ImGuiIO& io = ImGui::GetIO();
    const bool ctrl = io.KeyCtrl;

    // UI zoom shortcuts work even while typing in a text field.
    if (ctrl) {
        const bool zoomIn  = ImGui::IsKeyPressed(ImGuiKey_Equal,        false)
                          || ImGui::IsKeyPressed(ImGuiKey_KeypadAdd,    false);
        const bool zoomOut = ImGui::IsKeyPressed(ImGuiKey_Minus,        false)
                          || ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract, false);
        const bool reset   = ImGui::IsKeyPressed(ImGuiKey_0,            false)
                          || ImGui::IsKeyPressed(ImGuiKey_Keypad0,      false);
        if (zoomIn)  { m_uiScale = std::min(3.0f, m_uiScale * 1.1f); applyUiScale(); }
        if (zoomOut) { m_uiScale = std::max(0.5f, m_uiScale / 1.1f); applyUiScale(); }
        if (reset)   { m_uiScale = 1.0f;                              applyUiScale(); }
    }

    if (!m_ctx.scene) return;
    if (io.WantTextInput) return;

    if (!ctrl) {
        if (ImGui::IsKeyPressed(ImGuiKey_Q, false)) m_ctx.toolMode = ToolMode::Hand;
        if (ImGui::IsKeyPressed(ImGuiKey_W, false)) m_ctx.toolMode = ToolMode::Translate;
        if (ImGui::IsKeyPressed(ImGuiKey_E, false)) m_ctx.toolMode = ToolMode::Rotate;
        if (ImGui::IsKeyPressed(ImGuiKey_R, false)) m_ctx.toolMode = ToolMode::Scale;
    }

    if (m_ctx.selected != 0 && ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
        m_ctx.scene->removeEntity(m_ctx.selected);
        m_ctx.selected = 0;
    }
    if (m_ctx.selected != 0 && ctrl && ImGui::IsKeyPressed(ImGuiKey_D, false)) {
        if (auto* src = m_ctx.scene->findEntity(m_ctx.selected)) {
            auto& dup = m_ctx.scene->duplicateEntity(*src, src->name + " copy");
            m_ctx.selected = dup.id;
        }
    }
    if (m_ctx.selected != 0 && ctrl && ImGui::IsKeyPressed(ImGuiKey_C, false)) {
        if (auto* src = m_ctx.scene->findEntity(m_ctx.selected)) {
            m_ctx.clipboard = *src;
            m_ctx.clipboard->id = 0;
        }
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_V, false) && m_ctx.clipboard.has_value()) {
        auto& pasted = m_ctx.scene->duplicateEntity(*m_ctx.clipboard, m_ctx.clipboard->name);
        m_ctx.selected = pasted.id;
    }
}

} // namespace hopf::editor
