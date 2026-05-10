#include "hopf/editor/panels/HierarchyPanel.h"

#include "hopf/geometry/Primitives.h"
#include "hopf/scene/Scene.h"

#include <imgui.h>

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
        if (ImGui::MenuItem("Icosahedron")) addPrimitive(ctx, geometry::PrimitiveType::Icosahedron3D);
        if (ImGui::MenuItem("Sphere"))      addPrimitive(ctx, geometry::PrimitiveType::Sphere3D);
        if (ImGui::MenuItem("Cylinder"))    addPrimitive(ctx, geometry::PrimitiveType::Cylinder3D);
        if (ImGui::MenuItem("Cone"))        addPrimitive(ctx, geometry::PrimitiveType::Cone3D);
        if (ImGui::MenuItem("Torus"))       addPrimitive(ctx, geometry::PrimitiveType::Torus3D);
        if (ImGui::MenuItem("Pyramid"))     addPrimitive(ctx, geometry::PrimitiveType::Pyramid3D);
        if (ImGui::MenuItem("Prism"))       addPrimitive(ctx, geometry::PrimitiveType::Prism3D);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Add 4D")) {
        if (ImGui::MenuItem("Tesseract (8-cell)"))         addPrimitive(ctx, geometry::PrimitiveType::Tesseract);
        if (ImGui::MenuItem("Pentachoron (5-cell)"))       addPrimitive(ctx, geometry::PrimitiveType::Pentachoron);
        if (ImGui::MenuItem("Hexadecachoron (16-cell)"))   addPrimitive(ctx, geometry::PrimitiveType::Hexadecachoron);
        if (ImGui::MenuItem("Icositetrachoron (24-cell)")) addPrimitive(ctx, geometry::PrimitiveType::Icositetrachoron);
        if (ImGui::MenuItem("Tetrahedral prism"))          addPrimitive(ctx, geometry::PrimitiveType::TetrahedralPrism);
        if (ImGui::MenuItem("Cubical pyramid"))            addPrimitive(ctx, geometry::PrimitiveType::CubicalPyramid);
        ImGui::EndMenu();
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
    for (size_t i = 0; i < entities.size(); ++i) {
        const auto& e   = entities[i];
        const bool sel  = (ctx.selected == e.id);
        ImGui::PushID(static_cast<int>(e.id));
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
                auto& dup = ctx.scene->addEntity(e.name + " copy");
                dup.transform  = e.transform;
                dup.mesh       = e.mesh;
                dup.visible    = e.visible;
                dup.autoRotate = e.autoRotate;
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
                auto& pasted = ctx.scene->addEntity(ctx.clipboard->name);
                pasted.transform  = ctx.clipboard->transform;
                pasted.mesh       = ctx.clipboard->mesh;
                pasted.visible    = ctx.clipboard->visible;
                pasted.autoRotate = ctx.clipboard->autoRotate;
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
