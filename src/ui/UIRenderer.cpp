#include "ui/UIRenderer.h"

#include "scene/Scene.h"
#include "scripting/Script.h"

#include <imgui.h>

namespace hopf::ui {

namespace {

inline void anchor(scene::UIAnchor a, float x, float y, float w, float h,
                   float cmX, float cmY, float csX, float csY,
                   ImVec2& outMin, ImVec2& outMax)
{
    float mnX, mnY, mxX, mxY;
    resolveAnchor(a, x, y, w, h, cmX, cmY, csX, csY, mnX, mnY, mxX, mxY);
    outMin = ImVec2(mnX, mnY);
    outMax = ImVec2(mxX, mxY);
}

inline ImU32 packRGBA(float r, float g, float b, float a) {
    auto clamp01 = [](float x) { return x < 0.f ? 0.f : (x > 1.f ? 1.f : x); };
    return IM_COL32(int(clamp01(r) * 255.f),
                    int(clamp01(g) * 255.f),
                    int(clamp01(b) * 255.f),
                    int(clamp01(a) * 255.f));
}

} // namespace

void render(scene::Scene& scene,
            float canvasMinX, float canvasMinY,
            float canvasSizeX, float canvasSizeY,
            ImDrawList* dl,
            const RenderOptions& opts)
{
    if (!dl || canvasSizeX <= 0.f || canvasSizeY <= 0.f) return;

    const ImVec2 mouse   = ImGui::GetIO().MousePos;
    const bool   clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

    // Pass 1: draw rects and images (background layer).
    for (const auto& e : scene.entities()) {
        if (!e.visible) continue;
        if (e.uiRect.has_value()) {
            const auto& r = *e.uiRect;
            ImVec2 mn, mx;
            anchor(r.anchor, r.x, r.y, r.width, r.height,
                   canvasMinX, canvasMinY, canvasSizeX, canvasSizeY, mn, mx);
            dl->AddRectFilled(mn, mx, packRGBA(r.r, r.g, r.b, r.a), 4.f);
        }
        if (e.uiImage.has_value()) {
            const auto& im = *e.uiImage;
            scene::UIAnchor a = scene::UIAnchor::TopLeft;
            float           x = 0.f, y = 0.f, w = 100.f, h = 100.f;
            if (e.uiRect.has_value()) {
                a = e.uiRect->anchor; x = e.uiRect->x; y = e.uiRect->y;
                w = e.uiRect->width;  h = e.uiRect->height;
            }
            ImVec2 mn, mx;
            anchor(a, x, y, w, h, canvasMinX, canvasMinY, canvasSizeX, canvasSizeY, mn, mx);
            dl->AddRectFilled(mn, mx, packRGBA(im.r * 0.5f, im.g * 0.5f, im.b * 0.5f, im.a), 4.f);
            dl->AddRect(mn, mx, packRGBA(im.r, im.g, im.b, im.a), 4.f, 0, 2.f);
            dl->AddLine(mn, mx, packRGBA(im.r, im.g, im.b, im.a * 0.4f), 1.f);
            dl->AddLine(ImVec2(mn.x, mx.y), ImVec2(mx.x, mn.y),
                        packRGBA(im.r, im.g, im.b, im.a * 0.4f), 1.f);
        }
    }

    // Pass 2: buttons (foreground, hit-testable).
    for (auto& e : scene.entities()) {
        if (!e.visible || !e.uiButton.has_value()) continue;
        const auto& b = *e.uiButton;
        scene::UIAnchor a = scene::UIAnchor::Center;
        float           x = 0.f, y = 0.f, w = 200.f, h = 60.f;
        if (e.uiRect.has_value()) {
            a = e.uiRect->anchor; x = e.uiRect->x; y = e.uiRect->y;
            w = e.uiRect->width;  h = e.uiRect->height;
        }
        ImVec2 mn, mx;
        anchor(a, x, y, w, h, canvasMinX, canvasMinY, canvasSizeX, canvasSizeY, mn, mx);
        const bool hover = mouse.x >= mn.x && mouse.x <= mx.x
                        && mouse.y >= mn.y && mouse.y <= mx.y;
        const ImU32 fill = hover ? packRGBA(b.hoverR, b.hoverG, b.hoverB, b.hoverA)
                                 : packRGBA(b.r, b.g, b.b, b.a);
        dl->AddRectFilled(mn, mx, fill, 6.f);
        dl->AddRect(mn, mx, IM_COL32(0, 0, 0, 180), 6.f, 0, 1.5f);

        // Button label = co-located UIText if present, otherwise the entity name.
        const char* label = e.name.c_str();
        float       fontSize = 18.f;
        ImU32       textCol  = IM_COL32(255, 255, 255, 255);
        if (e.uiText.has_value()) {
            label    = e.uiText->text.c_str();
            fontSize = e.uiText->fontSize;
            textCol  = packRGBA(e.uiText->r, e.uiText->g, e.uiText->b, e.uiText->a);
        }
        ImFont* font = ImGui::GetFont();
        const ImVec2 size = font->CalcTextSizeA(fontSize, FLT_MAX, 0.f, label);
        const ImVec2 textPos((mn.x + mx.x - size.x) * 0.5f,
                             (mn.y + mx.y - size.y) * 0.5f);
        dl->AddText(font, fontSize, textPos, textCol, label);

        if (opts.interactive && hover && clicked && !b.onClickScript.empty()) {
            dispatchMessage(scene, b.onClickScript, b.onClickMethod);
        }
    }

    // Pass 3: standalone text (drawn last so it sits above rects/images but below
    // any button text that already drew). Buttons consume their co-located UIText.
    for (const auto& e : scene.entities()) {
        if (!e.visible || !e.uiText.has_value() || e.uiButton.has_value()) continue;
        const auto& t = *e.uiText;
        scene::UIAnchor a = scene::UIAnchor::Center;
        float           x = 0.f, y = 0.f, w = 400.f, h = t.fontSize * 1.4f;
        if (e.uiRect.has_value()) {
            a = e.uiRect->anchor; x = e.uiRect->x; y = e.uiRect->y;
            w = e.uiRect->width;  h = e.uiRect->height;
        }
        ImVec2 mn, mx;
        anchor(a, x, y, w, h, canvasMinX, canvasMinY, canvasSizeX, canvasSizeY, mn, mx);
        ImFont* font = ImGui::GetFont();
        const ImVec2 size = font->CalcTextSizeA(t.fontSize, FLT_MAX, 0.f, t.text.c_str());
        const ImVec2 textPos((mn.x + mx.x - size.x) * 0.5f,
                             (mn.y + mx.y - size.y) * 0.5f);
        dl->AddText(font, t.fontSize, textPos, packRGBA(t.r, t.g, t.b, t.a), t.text.c_str());
    }
}

} // namespace hopf::ui
