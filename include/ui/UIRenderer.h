#pragma once

// Game-overlay UI: walks the scene's UI components and draws them on top of the
// Game view image via an ImGui drawlist. Buttons hit-test the mouse and dispatch
// onClick to the named script type via Script::onMessage.
//
// The renderer reads the scene mutably because click dispatch can invoke scripts
// that mutate it. Pure data components (rect/text/image) are read-only.

#include "scene/Entity.h"

#include <string>

struct ImDrawList; // forward declare to keep imgui out of the public header.

namespace hopf::scene { class Scene; }

namespace hopf::ui {

// Resolves an anchored rectangle into screen-space top-left + bottom-right pixels.
// canvasMin* and canvasSize* are the Game view image rect in screen coords.
// x,y are the offset from the anchor point to the rectangle's CENTER (Unity/Godot
// convention).
void resolveAnchor(scene::UIAnchor anchor,
                   float x, float y, float width, float height,
                   float canvasMinX, float canvasMinY,
                   float canvasSizeX, float canvasSizeY,
                   float& outMinX, float& outMinY,
                   float& outMaxX, float& outMaxY);

// Finds the first script of `typeName` in the scene and calls onMessage(method).
// Returns true if a recipient was found, false otherwise.
bool dispatchMessage(scene::Scene& scene,
                     const std::string& typeName,
                     const std::string& method);

struct RenderOptions {
    // If true, button click hit-tests fire dispatch. The editor passes true only
    // while in Play mode; in Stopped/Paused the UI draws but stays inert.
    bool interactive = false;
};

void render(scene::Scene& scene,
            float canvasMinX, float canvasMinY,
            float canvasSizeX, float canvasSizeY,
            ImDrawList* dl,
            const RenderOptions& opts);

} // namespace hopf::ui
