#include "editor/panels/HierarchyPanel.h"

#include "geometry/Primitives.h"
#include "scene/Scene.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>

namespace hopf::editor {

namespace {

constexpr const char* kDragDropPayload = "HOPF_ENTITY";

void addPrimitive(EditorContext& ctx, geometry::PrimitiveType type) {
    if (!ctx.scene) return;
    auto& e = ctx.scene->addPrimitive(type);
    ctx.selected = e.id;
}

void renderAddObjectMenu(EditorContext& ctx) {
    if (ImGui::BeginMenu("Add 2D")) {
        if (ImGui::MenuItem("Triangle")) addPrimitive(ctx, geometry::PrimitiveType::Triangle2D);
        if (ImGui::MenuItem("Square"))   addPrimitive(ctx, geometry::PrimitiveType::Square2D);
        if (ImGui::MenuItem("Pentagon")) addPrimitive(ctx, geometry::PrimitiveType::Pentagon2D);
        if (ImGui::MenuItem("Hexagon"))  addPrimitive(ctx, geometry::PrimitiveType::Hexagon2D);
        if (ImGui::MenuItem("Star"))     addPrimitive(ctx, geometry::PrimitiveType::Star2D);
        if (ImGui::MenuItem("Circle"))   addPrimitive(ctx, geometry::PrimitiveType::Circle2D);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Add 3D")) {
        if (ImGui::MenuItem("Cube"))        addPrimitive(ctx, geometry::PrimitiveType::Cube3D);
        if (ImGui::MenuItem("Tetrahedron")) addPrimitive(ctx, geometry::PrimitiveType::Tetrahedron3D);
        if (ImGui::MenuItem("Octahedron"))  addPrimitive(ctx, geometry::PrimitiveType::Octahedron3D);
        if (ImGui::MenuItem("Icosahedron"))  addPrimitive(ctx, geometry::PrimitiveType::Icosahedron3D);
        if (ImGui::MenuItem("Dodecahedron")) addPrimitive(ctx, geometry::PrimitiveType::Dodecahedron3D);
        if (ImGui::MenuItem("Icosphere"))    addPrimitive(ctx, geometry::PrimitiveType::Icosphere3D);
        if (ImGui::MenuItem("Sphere"))       addPrimitive(ctx, geometry::PrimitiveType::Sphere3D);
        if (ImGui::MenuItem("Cylinder"))     addPrimitive(ctx, geometry::PrimitiveType::Cylinder3D);
        if (ImGui::MenuItem("Cone"))         addPrimitive(ctx, geometry::PrimitiveType::Cone3D);
        if (ImGui::MenuItem("Torus"))        addPrimitive(ctx, geometry::PrimitiveType::Torus3D);
        if (ImGui::MenuItem("Pyramid"))      addPrimitive(ctx, geometry::PrimitiveType::Pyramid3D);
        if (ImGui::MenuItem("Prism"))        addPrimitive(ctx, geometry::PrimitiveType::Prism3D);
        if (ImGui::MenuItem("Capsule"))      addPrimitive(ctx, geometry::PrimitiveType::Capsule3D);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Add Light")) {
        if (ImGui::MenuItem("Directional")) {
            if (ctx.scene) {
                auto& e = ctx.scene->addEntity("Light");
                e.light = scene::DirectionalLight{};
                ctx.selected = e.id;
            }
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Add Camera")) {
        if (ImGui::MenuItem("Camera2D") && ctx.scene) {
            auto& e = ctx.scene->addEntity("Camera2D");
            e.camera2D = scene::Camera2DComponent{};
            ctx.selected = e.id;
        }
        if (ImGui::MenuItem("Camera3D") && ctx.scene) {
            auto& e = ctx.scene->addEntity("Camera3D");
            e.camera3D = scene::Camera3DComponent{};
            ctx.selected = e.id;
        }
        if (ImGui::MenuItem("Camera4D") && ctx.scene) {
            auto& e = ctx.scene->addEntity("Camera4D");
            e.camera4D = scene::Camera4DComponent{};
            ctx.selected = e.id;
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Add 4D")) {
        if (ImGui::MenuItem("Tesseract (8-cell)"))         addPrimitive(ctx, geometry::PrimitiveType::Tesseract);
        if (ImGui::MenuItem("Pentachoron (5-cell)"))       addPrimitive(ctx, geometry::PrimitiveType::Pentachoron);
        if (ImGui::MenuItem("Hexadecachoron (16-cell)"))   addPrimitive(ctx, geometry::PrimitiveType::Hexadecachoron);
        if (ImGui::MenuItem("Icositetrachoron (24-cell)")) addPrimitive(ctx, geometry::PrimitiveType::Icositetrachoron);
        if (ImGui::MenuItem("Tetrahedral prism"))          addPrimitive(ctx, geometry::PrimitiveType::TetrahedralPrism);
        if (ImGui::MenuItem("Cubical pyramid"))            addPrimitive(ctx, geometry::PrimitiveType::CubicalPyramid);
        if (ImGui::MenuItem("Octahedral prism"))           addPrimitive(ctx, geometry::PrimitiveType::OctahedralPrism);
        if (ImGui::MenuItem("Icosahedral prism"))          addPrimitive(ctx, geometry::PrimitiveType::IcosahedralPrism);
        if (ImGui::MenuItem("Cubinder"))                   addPrimitive(ctx, geometry::PrimitiveType::Cubinder);
        if (ImGui::MenuItem("Spherinder"))                 addPrimitive(ctx, geometry::PrimitiveType::Spherinder);
        if (ImGui::MenuItem("Duoprism (3-3)"))             addPrimitive(ctx, geometry::PrimitiveType::Duoprism33);
        if (ImGui::MenuItem("Duoprism (5-5)"))             addPrimitive(ctx, geometry::PrimitiveType::Duoprism55);
        ImGui::EndMenu();
    }
}

enum class IconKind { Dim2, Dim3, Dim4, Light, Camera, Unknown };

IconKind classifyEntity(const scene::Entity& e) {
    if (e.light.has_value()) return IconKind::Light;
    if (e.camera2D.has_value() || e.camera3D.has_value() || e.camera4D.has_value())
        return IconKind::Camera;
    if (e.mesh) {
        const int d = scene::meshDimension(*e.mesh);
        if (d == 2) return IconKind::Dim2;
        if (d == 3) return IconKind::Dim3;
        return IconKind::Dim4;
    }
    return IconKind::Unknown;
}

void drawEntityIcon(ImDrawList* dl, ImVec2 min, ImVec2 max, IconKind kind) {
    const float w = max.x - min.x;
    const float h = max.y - min.y;
    const ImVec2 c{min.x + w * 0.5f, min.y + h * 0.5f};

    switch (kind) {
        case IconKind::Dim2: {
            // Orange triangle.
            const ImU32 col = IM_COL32(255, 152, 80, 255);
            const ImVec2 a{c.x,           min.y + h * 0.18f};
            const ImVec2 b{min.x + w * 0.10f, max.y - h * 0.18f};
            const ImVec2 cc{max.x - w * 0.10f, max.y - h * 0.18f};
            dl->AddTriangleFilled(a, b, cc, col);
            break;
        }
        case IconKind::Dim3: {
            // Cyan cube outline.
            const ImU32 col = IM_COL32(120, 200, 255, 255);
            const ImVec2 p0{min.x + w * 0.18f, min.y + h * 0.28f};
            const ImVec2 p1{max.x - w * 0.32f, min.y + h * 0.28f};
            const ImVec2 p2{max.x - w * 0.32f, max.y - h * 0.18f};
            const ImVec2 p3{min.x + w * 0.18f, max.y - h * 0.18f};
            const ImVec2 q0{min.x + w * 0.32f, min.y + h * 0.18f};
            const ImVec2 q1{max.x - w * 0.18f, min.y + h * 0.18f};
            const ImVec2 q2{max.x - w * 0.18f, max.y - h * 0.28f};
            dl->AddRect(p0, p2, col, 1.f, 0, 1.5f);
            dl->AddLine(p0, q0, col, 1.5f);
            dl->AddLine(p1, q1, col, 1.5f);
            dl->AddLine(p2, q2, col, 1.5f);
            dl->AddLine(q0, q1, col, 1.5f);
            dl->AddLine(q1, q2, col, 1.5f);
            break;
        }
        case IconKind::Dim4: {
            // Magenta "tesseract" (square in square).
            const ImU32 col = IM_COL32(220, 130, 220, 255);
            const ImVec2 oMin{min.x + w * 0.12f, min.y + h * 0.12f};
            const ImVec2 oMax{max.x - w * 0.12f, max.y - h * 0.12f};
            const ImVec2 iMin{min.x + w * 0.32f, min.y + h * 0.32f};
            const ImVec2 iMax{max.x - w * 0.32f, max.y - h * 0.32f};
            dl->AddRect(oMin, oMax, col, 1.f, 0, 1.5f);
            dl->AddRect(iMin, iMax, col, 1.f, 0, 1.5f);
            dl->AddLine(oMin,                 iMin,                 col, 1.f);
            dl->AddLine(ImVec2(oMax.x, oMin.y), ImVec2(iMax.x, iMin.y), col, 1.f);
            dl->AddLine(oMax,                 iMax,                 col, 1.f);
            dl->AddLine(ImVec2(oMin.x, oMax.y), ImVec2(iMin.x, iMax.y), col, 1.f);
            break;
        }
        case IconKind::Light: {
            // Yellow sun: filled circle + rays.
            const ImU32 col = IM_COL32(255, 213, 80, 255);
            const float r = std::min(w, h) * 0.22f;
            dl->AddCircleFilled(c, r, col, 12);
            const float R = std::min(w, h) * 0.42f;
            for (int i = 0; i < 8; ++i) {
                const float a = float(i) * 6.2831853f / 8.f;
                const float cosA = std::cos(a), sinA = std::sin(a);
                dl->AddLine(ImVec2(c.x + cosA * (r + 1.f),  c.y + sinA * (r + 1.f)),
                            ImVec2(c.x + cosA * R,          c.y + sinA * R),
                            col, 1.4f);
            }
            break;
        }
        case IconKind::Camera: {
            // Light gray "camera body" rectangle + lens.
            const ImU32 col = IM_COL32(190, 200, 210, 255);
            const ImVec2 bMin{min.x + w * 0.15f, min.y + h * 0.30f};
            const ImVec2 bMax{max.x - w * 0.35f, max.y - h * 0.25f};
            dl->AddRectFilled(bMin, bMax, col, 1.5f);
            dl->AddCircleFilled(ImVec2(max.x - w * 0.22f, c.y), std::min(w, h) * 0.18f, col, 12);
            break;
        }
        case IconKind::Unknown: {
            const ImU32 col = IM_COL32(140, 140, 150, 255);
            dl->AddCircleFilled(c, std::min(w, h) * 0.15f, col, 10);
            break;
        }
    }
}

} // namespace

void HierarchyPanel::render(EditorContext& ctx) {
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

    ImGui::TextUnformatted(ctx.scene->name().c_str());
    ImGui::Separator();

    scene::EntityId pendingDelete = 0;
    scene::EntityId moveSrc = 0, moveTarget = 0;

    auto& entities = ctx.scene->entities();
    const float iconSize = ImGui::GetTextLineHeight();
    for (size_t i = 0; i < entities.size(); ++i) {
        const auto& e   = entities[i];
        const bool sel  = (ctx.selected == e.id);
        ImGui::PushID(static_cast<int>(e.id));

        const ImVec2 iconMin = ImGui::GetCursorScreenPos();
        const ImVec2 iconMax{iconMin.x + iconSize, iconMin.y + iconSize};
        drawEntityIcon(ImGui::GetWindowDrawList(), iconMin, iconMax, classifyEntity(e));
        ImGui::Dummy(ImVec2(iconSize, iconSize));
        ImGui::SameLine(0.f, ImGui::GetStyle().ItemInnerSpacing.x);

        if (ImGui::Selectable(e.name.c_str(), sel)) {
            ctx.selected = e.id;
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload(kDragDropPayload, &e.id, sizeof(scene::EntityId));
            ImGui::TextUnformatted(e.name.c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
            if (auto* payload = ImGui::AcceptDragDropPayload(kDragDropPayload)) {
                moveSrc    = *static_cast<const scene::EntityId*>(payload->Data);
                moveTarget = e.id;
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::BeginPopupContextItem("##entityCtx")) {
            ctx.selected = e.id;
            if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
                auto& dup = ctx.scene->duplicateEntity(e, e.name + " copy");
                ctx.selected = dup.id;
            }
            if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                ctx.clipboard = e;
                ctx.clipboard->id = 0;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete", "Del")) pendingDelete = e.id;
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    if (ImGui::BeginPopupContextWindow("##hierarchyCtx",
                                       ImGuiPopupFlags_MouseButtonRight |
                                       ImGuiPopupFlags_NoOpenOverItems))
    {
        renderAddObjectMenu(ctx);
        if (ctx.clipboard.has_value()) {
            ImGui::Separator();
            if (ImGui::MenuItem("Paste", "Ctrl+V")) {
                auto& pasted = ctx.scene->duplicateEntity(*ctx.clipboard, ctx.clipboard->name);
                ctx.selected = pasted.id;
            }
        }
        ImGui::EndPopup();
    }

    if (pendingDelete != 0) {
        ctx.scene->removeEntity(pendingDelete);
        if (ctx.selected == pendingDelete) ctx.selected = 0;
    }
    if (moveSrc != 0 && moveTarget != 0 && moveSrc != moveTarget) {
        ctx.scene->moveEntityBefore(moveSrc, moveTarget);
    }

    ImGui::End();
}

} // namespace hopf::editor
