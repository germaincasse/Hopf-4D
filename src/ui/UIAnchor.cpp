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
    float ax = 0.f, ay = 0.f;
    switch (anchor) {
        case scene::UIAnchor::TopLeft:      ax = 0.f;             ay = 0.f;             break;
        case scene::UIAnchor::TopCenter:    ax = canvasSizeX*0.5f; ay = 0.f;            break;
        case scene::UIAnchor::TopRight:     ax = canvasSizeX;     ay = 0.f;             break;
        case scene::UIAnchor::MiddleLeft:   ax = 0.f;             ay = canvasSizeY*0.5f; break;
        case scene::UIAnchor::Center:       ax = canvasSizeX*0.5f; ay = canvasSizeY*0.5f; break;
        case scene::UIAnchor::MiddleRight:  ax = canvasSizeX;     ay = canvasSizeY*0.5f; break;
        case scene::UIAnchor::BottomLeft:   ax = 0.f;             ay = canvasSizeY;     break;
        case scene::UIAnchor::BottomCenter: ax = canvasSizeX*0.5f; ay = canvasSizeY;    break;
        case scene::UIAnchor::BottomRight:  ax = canvasSizeX;     ay = canvasSizeY;     break;
    }
    const float cx = canvasMinX + ax + x;
    const float cy = canvasMinY + ay + y;
    outMinX = cx - width  * 0.5f;
    outMinY = cy - height * 0.5f;
    outMaxX = cx + width  * 0.5f;
    outMaxY = cy + height * 0.5f;
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
