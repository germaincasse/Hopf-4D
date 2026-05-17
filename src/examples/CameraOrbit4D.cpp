// Camera orbit script: attach to a Main Camera entity to let the player rotate
// the 4D view with right-mouse drag and dolly closer/farther in 4D with the
// scroll wheel (changes wOffset in 4D-perspective mode, or sliceVal in Slice
// mode). Works only in Play mode; in Stop the camera transform is restored.

#include "scene/Entity.h"
#include "scene/Scene.h"
#include "scripting/Script.h"
#include "scripting/ScriptRegistry.h"

#include <algorithm>

namespace {

class CameraOrbit4D : public hopf::scripting::Script {
public:
    const char* typeName() const override { return "CameraOrbit4D"; }

    void onMouseDragged(float dx, float dy, int button) override {
        auto* e = entity();
        if (!e) return;
        if (button == 1) {
            // Right-drag: rotate around horizontal (xz) and vertical (yz) planes.
            e->transform.rotation.xz += dx * 0.005f;
            e->transform.rotation.yz += dy * 0.005f;
        } else if (button == 2) {
            // Middle-drag: rotate in the 4D xw / yw planes (slice through W).
            e->transform.rotation.xw += dx * 0.005f;
            e->transform.rotation.yw += dy * 0.005f;
        }
    }

    void onMouseWheel(float delta) override {
        auto* e = entity();
        if (!e || !e->camera4D.has_value()) return;
        auto& c = *e->camera4D;
        if (c.mode == hopf::scene::Camera4DComponent::Mode::Slice) {
            c.sliceVal += delta * 0.25f;
            c.sliceVal  = std::clamp(c.sliceVal, -10.f, 10.f);
        } else {
            // Dolly in/out along the camera's -W direction by tweaking wOffset.
            c.wOffset = std::clamp(c.wOffset - delta * 0.25f, -20.f, 20.f);
        }
    }
};

} // namespace

HOPF_REGISTER_SCRIPT(CameraOrbit4D)
