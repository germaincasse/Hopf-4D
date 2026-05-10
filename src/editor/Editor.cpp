#include "hopf/editor/Editor.h"

#include "hopf/editor/panels/ConsolePanel.h"
#include "hopf/editor/panels/FilesPanel.h"
#include "hopf/editor/panels/HierarchyPanel.h"
#include "hopf/editor/panels/InspectorPanel.h"
#include "hopf/editor/panels/ViewportPanel.h"

#include "hopf/scene/Scene.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

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

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    m_panels.emplace_back(std::make_unique<ViewportPanel>());
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

    ImGuiID center = dockspaceId;
    ImGuiID right  = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.22f, nullptr, &center);
    ImGuiID left   = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left,  0.20f, nullptr, &center);
    ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down,  0.25f, nullptr, &center);

    ImGui::DockBuilderDockWindow("Viewport (4D)", center);
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
            if (ImGui::MenuItem("Reset layout")) m_layoutBuilt = false;
            ImGui::EndMenu();
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

void Editor::processShortcuts() {
    if (!m_ctx.scene) return;
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput) return;

    const bool ctrl = io.KeyCtrl;

    if (m_ctx.selected != 0 && ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
        m_ctx.scene->removeEntity(m_ctx.selected);
        m_ctx.selected = 0;
    }
    if (m_ctx.selected != 0 && ctrl && ImGui::IsKeyPressed(ImGuiKey_D, false)) {
        if (auto* src = m_ctx.scene->findEntity(m_ctx.selected)) {
            auto& dup = m_ctx.scene->addEntity(src->name + " copy");
            dup.transform  = src->transform;
            dup.mesh       = src->mesh;
            dup.visible    = src->visible;
            dup.autoRotate = src->autoRotate;
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
        auto& pasted = m_ctx.scene->addEntity(m_ctx.clipboard->name);
        pasted.transform  = m_ctx.clipboard->transform;
        pasted.mesh       = m_ctx.clipboard->mesh;
        pasted.visible    = m_ctx.clipboard->visible;
        pasted.autoRotate = m_ctx.clipboard->autoRotate;
        m_ctx.selected = pasted.id;
    }
}

} // namespace hopf::editor
