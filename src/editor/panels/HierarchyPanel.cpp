#include "editor/panels/HierarchyPanel.h"

#include "geometry/Primitives.h"
#include "scene/Scene.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <unordered_map>

namespace hopf::editor {

namespace {

constexpr const char* kDragDropPayload = "HOPF_ENTITY";

void addPrimitive(EditorContext& ctx, geometry::PrimitiveType type) {
    if (!ctx.scene) return;
    ctx.pushUndo();
    auto& e = ctx.scene->addPrimitive(type);
    ctx.selected = e.id;
}

void addEmpty(EditorContext& ctx, const char* name) {
    if (!ctx.scene) return;
    ctx.pushUndo();
    auto& e = ctx.scene->addEntity(name);
    ctx.selected = e.id;
}

void renderAddObjectMenu(EditorContext& ctx) {
    if (ImGui::BeginMenu("Empty")) {
        if (ImGui::MenuItem("Empty 2D")) addEmpty(ctx, "Empty 2D");
        if (ImGui::MenuItem("Empty 3D")) addEmpty(ctx, "Empty 3D");
        if (ImGui::MenuItem("Empty 4D")) addEmpty(ctx, "Empty 4D");
        if (ImGui::MenuItem("Empty UI")) addEmpty(ctx, "UI Root");
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("2D")) {
        if (ImGui::MenuItem("Triangle")) addPrimitive(ctx, geometry::PrimitiveType::Triangle2D);
        if (ImGui::MenuItem("Square"))   addPrimitive(ctx, geometry::PrimitiveType::Square2D);
        if (ImGui::MenuItem("Pentagon")) addPrimitive(ctx, geometry::PrimitiveType::Pentagon2D);
        if (ImGui::MenuItem("Hexagon"))  addPrimitive(ctx, geometry::PrimitiveType::Hexagon2D);
        if (ImGui::MenuItem("Star"))     addPrimitive(ctx, geometry::PrimitiveType::Star2D);
        if (ImGui::MenuItem("Circle"))   addPrimitive(ctx, geometry::PrimitiveType::Circle2D);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("3D")) {
        if (ImGui::MenuItem("Cube"))         addPrimitive(ctx, geometry::PrimitiveType::Cube3D);
        if (ImGui::MenuItem("Tetrahedron"))  addPrimitive(ctx, geometry::PrimitiveType::Tetrahedron3D);
        if (ImGui::MenuItem("Octahedron"))   addPrimitive(ctx, geometry::PrimitiveType::Octahedron3D);
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
    if (ImGui::BeginMenu("4D")) {
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
    if (ImGui::BeginMenu("UI")) {
        if (ImGui::MenuItem("Rectangle")) {
            if (ctx.scene) {
                ctx.pushUndo();
                auto& e = ctx.scene->addEntity("Rectangle");
                e.uiRect = scene::UIRectComponent{};
                ctx.selected = e.id;
            }
        }
        if (ImGui::MenuItem("Text")) {
            if (ctx.scene) {
                ctx.pushUndo();
                auto& e = ctx.scene->addEntity("Text");
                e.uiText = scene::UITextComponent{};
                ctx.selected = e.id;
            }
        }
        if (ImGui::MenuItem("Image")) {
            if (ctx.scene) {
                ctx.pushUndo();
                auto& e = ctx.scene->addEntity("Image");
                e.uiImage = scene::UIImageComponent{};
                e.uiRect  = scene::UIRectComponent{}; // images need a rect for size/position
                ctx.selected = e.id;
            }
        }
        if (ImGui::MenuItem("Button")) {
            if (ctx.scene) {
                ctx.pushUndo();
                auto& e = ctx.scene->addEntity("Button");
                e.uiRect   = scene::UIRectComponent{};
                e.uiButton = scene::UIButtonComponent{};
                e.uiText   = scene::UITextComponent{};
                e.uiText->text = "Button";
                ctx.selected = e.id;
            }
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Audio")) {
        if (ImGui::MenuItem("Audio Source")) {
            if (ctx.scene) {
                ctx.pushUndo();
                auto& e = ctx.scene->addEntity("Audio Source");
                e.audioSource = scene::AudioSourceComponent{};
                ctx.selected = e.id;
            }
        }
        if (ImGui::MenuItem("Audio Listener")) {
            if (ctx.scene) {
                ctx.pushUndo();
                auto& e = ctx.scene->addEntity("Audio Listener");
                e.audioListener = scene::AudioListenerComponent{};
                ctx.selected = e.id;
            }
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Light")) {
        if (ImGui::MenuItem("Directional")) {
            if (ctx.scene) {
                ctx.pushUndo();
                auto& e = ctx.scene->addEntity("Light");
                e.light = scene::DirectionalLight{};
                ctx.selected = e.id;
            }
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Camera")) {
        if (ImGui::MenuItem("Camera 2D") && ctx.scene) {
            ctx.pushUndo();
            auto& e = ctx.scene->addEntity("Camera 2D");
            e.camera2D = scene::Camera2DComponent{};
            ctx.selected = e.id;
        }
        if (ImGui::MenuItem("Camera 3D") && ctx.scene) {
            ctx.pushUndo();
            auto& e = ctx.scene->addEntity("Camera 3D");
            e.camera3D = scene::Camera3DComponent{};
            ctx.selected = e.id;
        }
        if (ImGui::MenuItem("Camera 4D") && ctx.scene) {
            ctx.pushUndo();
            auto& e = ctx.scene->addEntity("Camera 4D");
            e.camera4D = scene::Camera4DComponent{};
            bool anyMain = false;
            for (const auto& other : ctx.scene->entities()) {
                if (other.id != e.id && other.camera4D.has_value() && other.camera4D->isMain) {
                    anyMain = true;
                    break;
                }
            }
            if (!anyMain) e.camera4D->isMain = true;
            ctx.selected = e.id;
        }
        ImGui::EndMenu();
    }
}

enum class IconKind { Dim2, Dim3, Dim4, Light, Camera, UI, Empty, Unknown };

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
    if (e.uiRect.has_value() || e.uiText.has_value()
        || e.uiImage.has_value() || e.uiButton.has_value()) return IconKind::UI;
    return IconKind::Empty;
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
        case IconKind::UI: {
            // Two stacked rectangles suggesting overlapping panels.
            const ImU32 col = IM_COL32(150, 200, 130, 255);
            const ImVec2 a0(min.x + w * 0.15f, min.y + h * 0.25f);
            const ImVec2 a1(max.x - w * 0.30f, max.y - h * 0.15f);
            const ImVec2 b0(min.x + w * 0.35f, min.y + h * 0.15f);
            const ImVec2 b1(max.x - w * 0.15f, max.y - h * 0.30f);
            dl->AddRectFilled(a0, a1, IM_COL32(60, 80, 50, 200), 1.5f);
            dl->AddRect(a0, a1, col, 1.5f, 0, 1.4f);
            dl->AddRectFilled(b0, b1, IM_COL32(60, 80, 50, 200), 1.5f);
            dl->AddRect(b0, b1, col, 1.5f, 0, 1.4f);
            break;
        }
        case IconKind::Empty: {
            // A hollow circle with a tiny plus inside -- "empty slot".
            const ImU32 col = IM_COL32(160, 160, 170, 230);
            const float radius = std::min(w, h) * 0.32f;
            dl->AddCircle(c, radius, col, 18, 1.4f);
            const float p = radius * 0.5f;
            dl->AddLine(ImVec2(c.x - p, c.y), ImVec2(c.x + p, c.y), col, 1.2f);
            dl->AddLine(ImVec2(c.x, c.y - p), ImVec2(c.x, c.y + p), col, 1.2f);
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
    scene::EntityId reparentSrc = 0, reparentTo = 0;
    bool reparentIntent = false;
    scene::EntityId reorderSrc = 0, reorderBefore = 0;
    bool reorderIntent = false;

    auto& entities = ctx.scene->entities();
    const float iconSize = ImGui::GetTextLineHeight();

    // Pre-compute child lists for tree walk.
    std::unordered_map<scene::EntityId, std::vector<size_t>> childrenOf;
    std::vector<size_t> roots;
    for (size_t i = 0; i < entities.size(); ++i) {
        const auto& e = entities[i];
        if (e.parent == 0 || ctx.scene->findEntity(e.parent) == nullptr) {
            roots.push_back(i);
        } else {
            childrenOf[e.parent].push_back(i);
        }
    }

    std::function<void(size_t)> renderRow = [&](size_t idx) {
        const auto& e   = entities[idx];
        const bool sel  = (ctx.selected == e.id);
        const auto& kids = childrenOf[e.id];

        ImGui::PushID(static_cast<int>(e.id));

        // Thin drop target ABOVE the row: dropping here reorders the dragged entity
        // to sit just before this row (sibling reorder).
        ImGui::InvisibleButton("##gap", ImVec2(-1.f, 4.f));
        if (ImGui::BeginDragDropTarget()) {
            if (auto* payload = ImGui::AcceptDragDropPayload(kDragDropPayload)) {
                reorderSrc    = *static_cast<const scene::EntityId*>(payload->Data);
                reorderBefore = e.id;
                reorderIntent = true;
            }
            ImGui::EndDragDropTarget();
        }

        // Icon strip.
        const ImVec2 iconMin = ImGui::GetCursorScreenPos();
        const ImVec2 iconMax{iconMin.x + iconSize, iconMin.y + iconSize};
        drawEntityIcon(ImGui::GetWindowDrawList(), iconMin, iconMax, classifyEntity(e));
        ImGui::Dummy(ImVec2(iconSize, iconSize));
        ImGui::SameLine(0.f, ImGui::GetStyle().ItemInnerSpacing.x);

        ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow
                                      | ImGuiTreeNodeFlags_OpenOnDoubleClick
                                      | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (sel)         nodeFlags |= ImGuiTreeNodeFlags_Selected;
        if (kids.empty()) nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        const bool opened = ImGui::TreeNodeEx("##row", nodeFlags, "%s", e.name.c_str());
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            ctx.selected = e.id;
        }

        // Drag source.
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload(kDragDropPayload, &e.id, sizeof(scene::EntityId));
            ImGui::TextUnformatted(e.name.c_str());
            ImGui::EndDragDropSource();
        }
        // Drag target: dropping an entity onto this row reparents it under this one.
        if (ImGui::BeginDragDropTarget()) {
            if (auto* payload = ImGui::AcceptDragDropPayload(kDragDropPayload)) {
                reparentSrc    = *static_cast<const scene::EntityId*>(payload->Data);
                reparentTo     = e.id;
                reparentIntent = true;
            }
            ImGui::EndDragDropTarget();
        }

        // Context menu.
        if (ImGui::BeginPopupContextItem("##entityCtx")) {
            ctx.selected = e.id;
            if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
                ctx.pushUndo();
                auto& dup = ctx.scene->duplicateEntity(e, e.name + " copy");
                ctx.selected = dup.id;
            }
            if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                ctx.clipboard = e;
                ctx.clipboard->id = 0;
            }
            if (e.parent != 0) {
                if (ImGui::MenuItem("Unparent")) {
                    reparentSrc    = e.id;
                    reparentTo     = 0;
                    reparentIntent = true;
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete", "Del")) pendingDelete = e.id;
            ImGui::EndPopup();
        }

        if (opened && !kids.empty()) {
            for (size_t ci : kids) renderRow(ci);
            ImGui::TreePop();
        }
        ImGui::PopID();
    };

    for (size_t ri : roots) renderRow(ri);

    // Empty area at the bottom: drop here to unparent.
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    if (avail.y > 10.f) {
        ImGui::Dummy(ImVec2(avail.x, std::max(20.f, avail.y - 4.f)));
        if (ImGui::BeginDragDropTarget()) {
            if (auto* payload = ImGui::AcceptDragDropPayload(kDragDropPayload)) {
                reparentSrc    = *static_cast<const scene::EntityId*>(payload->Data);
                reparentTo     = 0;
                reparentIntent = true;
            }
            ImGui::EndDragDropTarget();
        }
    }

    if (ImGui::BeginPopupContextWindow("##hierarchyCtx",
                                       ImGuiPopupFlags_MouseButtonRight |
                                       ImGuiPopupFlags_NoOpenOverItems))
    {
        renderAddObjectMenu(ctx);
        if (ctx.clipboard.has_value()) {
            ImGui::Separator();
            if (ImGui::MenuItem("Paste", "Ctrl+V")) {
                ctx.pushUndo();
                auto& pasted = ctx.scene->duplicateEntity(*ctx.clipboard, ctx.clipboard->name);
                ctx.selected = pasted.id;
            }
        }
        ImGui::EndPopup();
    }

    if (pendingDelete != 0) {
        ctx.pushUndo();
        ctx.scene->removeEntity(pendingDelete);
        if (ctx.selected == pendingDelete) ctx.selected = 0;
    }
    if (reparentIntent && reparentSrc != 0 && reparentSrc != reparentTo) {
        ctx.pushUndo();
        ctx.scene->setParent(reparentSrc, reparentTo);
    }
    if (reorderIntent && reorderSrc != 0 && reorderSrc != reorderBefore) {
        ctx.pushUndo();
        // Reorder siblings: also make the dragged entity a sibling of `reorderBefore`
        // by adopting its parent.
        if (auto* target = ctx.scene->findEntity(reorderBefore)) {
            ctx.scene->setParent(reorderSrc, target->parent);
        }
        ctx.scene->moveEntityBefore(reorderSrc, reorderBefore);
    }

    ImGui::End();
}

} // namespace hopf::editor
