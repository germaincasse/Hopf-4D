// Imgui-free portion of the UI module: anchor math + message dispatch. Lives
// in its own TU so the test suite can exercise it without pulling in ImGui/GLFW.

#include "ui/UIRenderer.h"

#include "scene/Scene.h"
#include "scripting/Script.h"

namespace hopf::ui {

void resolveAnchor(scene::UIAnchor anchor,
                   float x, float y, float width, float height,
                   float canvasMinX, float canvasMinY,
                   float canvasSizeX, float canvasSizeY,
                   float& outMinX, float& outMinY,
                   float& outMaxX, float& outMaxY)
{
    // Anchor point on the canvas + pivot fraction on the rect itself. For TopLeft
    // anchor the rect's top-left corner sits at anchor + (x, y); for Center, the
    // rect's center; for BottomRight, the rect's bottom-right corner; and so on.
    float ax = 0.f, ay = 0.f;        // anchor position in the canvas
    float px = 0.f, py = 0.f;        // pivot on the rect (0 = min edge, 1 = max edge)
    switch (anchor) {
        case scene::UIAnchor::TopLeft:      ax = 0.f;             ay = 0.f;              px = 0.f; py = 0.f; break;
        case scene::UIAnchor::TopCenter:    ax = canvasSizeX*0.5f; ay = 0.f;             px = 0.5f; py = 0.f; break;
        case scene::UIAnchor::TopRight:     ax = canvasSizeX;     ay = 0.f;              px = 1.f; py = 0.f; break;
        case scene::UIAnchor::MiddleLeft:   ax = 0.f;             ay = canvasSizeY*0.5f; px = 0.f; py = 0.5f; break;
        case scene::UIAnchor::Center:       ax = canvasSizeX*0.5f; ay = canvasSizeY*0.5f; px = 0.5f; py = 0.5f; break;
        case scene::UIAnchor::MiddleRight:  ax = canvasSizeX;     ay = canvasSizeY*0.5f; px = 1.f; py = 0.5f; break;
        case scene::UIAnchor::BottomLeft:   ax = 0.f;             ay = canvasSizeY;      px = 0.f; py = 1.f; break;
        case scene::UIAnchor::BottomCenter: ax = canvasSizeX*0.5f; ay = canvasSizeY;     px = 0.5f; py = 1.f; break;
        case scene::UIAnchor::BottomRight:  ax = canvasSizeX;     ay = canvasSizeY;      px = 1.f; py = 1.f; break;
    }
    const float pivotScreenX = canvasMinX + ax + x;
    const float pivotScreenY = canvasMinY + ay + y;
    outMinX = pivotScreenX - width  * px;
    outMinY = pivotScreenY - height * py;
    outMaxX = outMinX + width;
    outMaxY = outMinY + height;
}

bool dispatchMessage(scene::Scene& scene,
                     const std::string& typeName,
                     const std::string& method)
{
    for (auto& e : scene.entities()) {
        for (auto& s : e.scripts) {
            if (s.typeName == typeName && s.instance) {
                s.instance->onMessage(method);
                return true;
            }
        }
    }
    return false;
}

} // namespace hopf::ui
