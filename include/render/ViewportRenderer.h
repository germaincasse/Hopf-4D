#pragma once

#include "render/Camera4D.h"
#include "render/Shader.h"

#include <array>
#include <cstdint>

namespace hopf::scene { class Scene; }

namespace hopf::render {

// Screen-space tip of one gizmo axis after view + projection. Used by the editor to
// overlay axis letter labels on top of the rendered framebuffer.
struct AxisScreenLabel {
    float u       = 0.f;  // pixel coords inside the viewport image (top-left origin)
    float v       = 0.f;
    bool  visible = false;
};

// Renders a scene to an off-screen framebuffer; the resulting color texture is
// displayed by the editor's Viewport panel.
class ViewportRenderer {
public:
    ViewportRenderer();
    ~ViewportRenderer();

    ViewportRenderer(const ViewportRenderer&) = delete;
    ViewportRenderer& operator=(const ViewportRenderer&) = delete;

    void resize(int width, int height);
    void render(const scene::Scene& scene, const Camera4D& camera);

    uint32_t colorTexture() const { return m_colorTex; }
    int width()  const { return m_width; }
    int height() const { return m_height; }

    // Indexed by axis: 0=X, 1=Y, 2=Z, 3=W. Populated during render().
    const std::array<AxisScreenLabel, 4>& axisLabels() const { return m_axisLabels; }

private:
    void ensureGpuResources();
    void destroyFramebuffer();
    void rebuildFramebuffer();

    int      m_width  = 0;
    int      m_height = 0;

    uint32_t m_fbo     = 0;
    uint32_t m_colorTex = 0;
    uint32_t m_depthRb  = 0;

    uint32_t m_lineVao = 0;
    uint32_t m_lineVbo = 0;
    uint32_t m_lineEbo = 0;

    uint32_t m_triVao = 0;
    uint32_t m_triVbo = 0;
    uint32_t m_triEbo = 0;

    Shader m_lineShader;
    Shader m_triShader;
    bool   m_shadersReady = false;

    std::array<AxisScreenLabel, 4> m_axisLabels{};
};

} // namespace hopf::render
