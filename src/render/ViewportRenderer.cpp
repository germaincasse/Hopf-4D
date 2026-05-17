#include "render/ViewportRenderer.h"

#include "render/Projector.h"
#include "render/Slicer.h"
#include "render/ViewportShaders.h"
#include "core/Logger.h"
#include "math/Mat4.h"
#include "scene/Scene.h"

#include <glad/gl.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace hopf::render {

namespace {

// 4D -> 3D projection consistent with how meshes are rendered. In Projection mode
// the result is (x, y, z) (optionally w-perspective-scaled). In Slice mode the result
// is the three components other than the slice axis, in axis order.
std::array<float, 3> project4to3(const math::Vec4& v, const Camera4D& camera) {
    if (camera.mode == ViewMode::Projection) {
        if (camera.projectionStyle == ProjectionStyle::Perspective) {
            const float wp = v.w + camera.wOffset;
            const float k  = wp != 0.f ? camera.focal4 / wp : 1.f;
            return { v.x * k, v.y * k, v.z * k };
        }
        return { v.x, v.y, v.z };
    }
    const int axis = static_cast<int>(camera.sliceAxis);
    std::array<float, 3> out{};
    int oi = 0;
    for (int i = 0; i < 4; ++i) {
        if (i != axis) out[oi++] = v[i];
    }
    return out;
}

// Like project4to3 but for a direction vector: no perspective scaling. Used to bring
// the directional light into the same 3D space as the rendered geometry's normals.
std::array<float, 3> projectDir4to3(const math::Vec4& d, const Camera4D& camera) {
    if (camera.mode == ViewMode::Projection) {
        return { d.x, d.y, d.z };
    }
    const int axis = static_cast<int>(camera.sliceAxis);
    std::array<float, 3> out{};
    int oi = 0;
    for (int i = 0; i < 4; ++i) {
        if (i != axis) out[oi++] = d[i];
    }
    return out;
}

void drawWorldAxes(uint32_t vao, uint32_t vbo, uint32_t ebo, Shader& s,
                   const math::Mat5& viewFromWorld, const Camera4D& camera)
{
    const float L = 1.f;
    const math::Vec4 axisLocal[4] = {
        {L, 0, 0, 0},
        {0, L, 0, 0},
        {0, 0, L, 0},
        {0, 0, 0, L},
    };
    const float colors[4][3] = {
        {1.0f, 0.30f, 0.30f},
        {0.3f, 1.00f, 0.30f},
        {0.3f, 0.55f, 1.00f},
        {1.0f, 0.85f, 0.20f},
    };

    // In Slice mode the axis perpendicular to the hyperplane collapses to a point;
    // skip drawing it. In Projection mode all 4 axes are drawn.
    const int hideAxis = (camera.mode == ViewMode::Slice)
                       ? static_cast<int>(camera.sliceAxis)
                       : -1;

    const math::Vec4 originView = viewFromWorld.transformPoint({0, 0, 0, 0});
    const auto       originProj = project4to3(originView, camera);

    struct V { float x, y, z, w; };
    V        verts[8];
    uint32_t idx[8];
    int slotCount = 0;
    int slot[4]   = { -1, -1, -1, -1 };
    for (int i = 0; i < 4; ++i) {
        if (i == hideAxis) continue;
        const math::Vec4 endView = viewFromWorld.transformPoint(axisLocal[i]);
        const auto       endProj = project4to3(endView, camera);
        verts[slotCount * 2 + 0] = {originProj[0], originProj[1], originProj[2], originView.w};
        verts[slotCount * 2 + 1] = {endProj[0],    endProj[1],    endProj[2],    endView.w};
        idx[slotCount * 2 + 0] = static_cast<uint32_t>(slotCount * 2 + 0);
        idx[slotCount * 2 + 1] = static_cast<uint32_t>(slotCount * 2 + 1);
        slot[i] = slotCount;
        ++slotCount;
    }

    if (slotCount == 0) return;

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(slotCount * 2 * sizeof(V)),
                 verts, GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(slotCount * 2 * sizeof(uint32_t)),
                 idx, GL_STREAM_DRAW);

    s.bind();
    s.setInt("uDepthShade", 0);
    s.setFloat("uAlpha", 1.f);
    for (int i = 0; i < 4; ++i) {
        if (slot[i] < 0) continue;
        s.setVec3("uColor", colors[i][0], colors[i][1], colors[i][2]);
        glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT,
                       (void*)(static_cast<intptr_t>(slot[i] * 2 * sizeof(uint32_t))));
    }
}

void drawGrids(uint32_t vao, uint32_t vbo, uint32_t ebo, Shader& s,
               const math::Mat5& viewFromWorld, const Camera4D& camera)
{
    const auto& g = camera.grid;
    const bool any = g.showXY || g.showXZ || g.showYZ
                  || g.showXW || g.showYW || g.showZW;
    if (!any || g.opacity <= 0.f) return;

    const int   halfN = std::max(1, g.cells / 2);
    constexpr float step  = 1.f;
    const float ext = float(halfN) * step;

    struct V { float x, y, z, w; };
    std::vector<V>        verts;
    std::vector<uint32_t> idx;

    auto addPlaneGrid = [&](int axis1, int axis2) {
        for (int i = -halfN; i <= halfN; ++i) {
            const float a = float(i) * step;
            // Two perpendicular line families through (a1=a, axis2 spans ±ext)
            // and (axis2=a, axis1 spans ±ext).
            math::Vec4 p[4]{};
            p[0][axis1] =  a;     p[0][axis2] = -ext;
            p[1][axis1] =  a;     p[1][axis2] =  ext;
            p[2][axis2] =  a;     p[2][axis1] = -ext;
            p[3][axis2] =  a;     p[3][axis1] =  ext;
            for (int k = 0; k < 4; ++k) {
                const auto pp = project4to3(viewFromWorld.transformPoint(p[k]), camera);
                verts.push_back({pp[0], pp[1], pp[2], 0.f});
            }
        }
    };

    if (g.showXY) addPlaneGrid(0, 1);
    if (g.showXZ) addPlaneGrid(0, 2);
    if (g.showYZ) addPlaneGrid(1, 2);
    if (g.showXW) addPlaneGrid(0, 3);
    if (g.showYW) addPlaneGrid(1, 3);
    if (g.showZW) addPlaneGrid(2, 3);

    idx.reserve(verts.size());
    for (uint32_t i = 0; i < verts.size(); ++i) idx.push_back(i);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(verts.size() * sizeof(V)),
                 verts.data(), GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(idx.size() * sizeof(uint32_t)),
                 idx.data(), GL_STREAM_DRAW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    s.bind();
    s.setInt("uDepthShade", 0);
    s.setVec3("uColor", 0.45f, 0.45f, 0.50f);
    s.setFloat("uAlpha", g.opacity);
    glDrawElements(GL_LINES, (GLsizei)idx.size(), GL_UNSIGNED_INT, nullptr);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

} // namespace

ViewportRenderer::ViewportRenderer() = default;

ViewportRenderer::~ViewportRenderer() {
    destroyFramebuffer();

    if (m_lineEbo) glDeleteBuffers(1, &m_lineEbo);
    if (m_lineVbo) glDeleteBuffers(1, &m_lineVbo);
    if (m_lineVao) glDeleteVertexArrays(1, &m_lineVao);

    if (m_triVbo) glDeleteBuffers(1, &m_triVbo);
    if (m_triVao) glDeleteVertexArrays(1, &m_triVao);
}

void ViewportRenderer::destroyFramebuffer() {
    if (m_depthRb)  { glDeleteRenderbuffers(1, &m_depthRb);  m_depthRb  = 0; }
    if (m_colorTex) { glDeleteTextures(1,    &m_colorTex);   m_colorTex = 0; }
    if (m_fbo)      { glDeleteFramebuffers(1, &m_fbo);       m_fbo      = 0; }
}

void ViewportRenderer::rebuildFramebuffer() {
    destroyFramebuffer();
    if (m_width <= 0 || m_height <= 0) return;

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glGenTextures(1, &m_colorTex);
    glBindTexture(GL_TEXTURE_2D, m_colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTex, 0);

    glGenRenderbuffers(1, &m_depthRb);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthRb);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        core::Logger::error("Viewport framebuffer incomplete");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ViewportRenderer::ensureGpuResources() {
    if (!m_lineVao) {
        glGenVertexArrays(1, &m_lineVao);
        glGenBuffers(1, &m_lineVbo);
        glGenBuffers(1, &m_lineEbo);

        glBindVertexArray(m_lineVao);
        glBindBuffer(GL_ARRAY_BUFFER, m_lineVbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_lineEbo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(0));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
    }

    if (!m_triVao) {
        glGenVertexArrays(1, &m_triVao);
        glGenBuffers(1, &m_triVbo);

        glBindVertexArray(m_triVao);
        glBindBuffer(GL_ARRAY_BUFFER, m_triVbo);
        // Layout: vec3 pos at offset 0, vec3 flat normal at offset 12.
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    }

    if (!m_shadersReady) {
        m_lineShader.compile(shaders::kLineVS, shaders::kLineFS);
        m_triShader.compile(shaders::kTriVS, shaders::kTriFS);
        m_shadersReady = true;
    }
}

void ViewportRenderer::resize(int width, int height) {
    width  = std::max(1, width);
    height = std::max(1, height);
    if (width == m_width && height == m_height && m_fbo != 0) return;
    m_width  = width;
    m_height = height;
    rebuildFramebuffer();
}

void ViewportRenderer::render(const scene::Scene& scene, const Camera4D& camera) {
    ensureGpuResources();
    if (m_fbo == 0) return;

    const float yaw   = camera.yaw;
    const float pitch = camera.pitch;
    const float dist  = std::max(0.1f, camera.distance);
    const float dirX = std::cos(pitch) * std::sin(yaw);
    const float dirY = std::sin(pitch);
    const float dirZ = std::cos(pitch) * std::cos(yaw);
    const float ex = camera.pivotX + dist * dirX;
    const float ey = camera.pivotY + dist * dirY;
    const float ez = camera.pivotZ + dist * dirZ;
    // Flip world-up when crossing the poles so lookAt stays well-defined and orbit
    // remains continuous through 360deg.
    const float upY = std::cos(pitch) >= 0.f ? 1.f : -1.f;

    const float aspect  = float(m_width) / float(m_height);
    const float fovYRad = camera.fovYDeg * 3.14159265f / 180.f;
    math::Mat4 P;
    if (camera.projectionStyle == ProjectionStyle::Perspective) {
        P = math::perspective(fovYRad, aspect, camera.zNear, camera.zFar);
    } else {
        // Match the perspective view's apparent size at the pivot: the visible vertical
        // half-extent at distance `dist` is `dist * tan(fovY/2)`.
        const float halfH = std::max(0.05f, dist) * std::tan(fovYRad * 0.5f);
        const float halfW = halfH * aspect;
        P = math::ortho(-halfW, halfW, -halfH, halfH, camera.zNear, camera.zFar);
    }
    const math::Mat4 V = math::lookAt(ex, ey, ez,
                                      camera.pivotX, camera.pivotY, camera.pivotZ,
                                      0.f, upY, 0.f);
    const math::Mat4 VP = math::mat4Mul(P, V);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);
    glClearColor(camera.bgR, camera.bgG, camera.bgB, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(1.5f);
    // 4D projection produces both-facing triangles; never cull.
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    const math::Mat5 viewFromWorld = camera.rotation4.toInverseMatrix()
                                   * math::translate4({-camera.position4.x,
                                                       -camera.position4.y,
                                                       -camera.position4.z,
                                                       -camera.position4.w});

    m_lineShader.bind();
    m_lineShader.setMat4("uViewProj", VP.data());
    drawGrids(m_lineVao, m_lineVbo, m_lineEbo, m_lineShader, viewFromWorld, camera);
    drawWorldAxes(m_lineVao, m_lineVbo, m_lineEbo, m_lineShader, viewFromWorld, camera);

    {
        const float L = 1.f;
        const math::Vec4 axisLocal[4] = {
            {L, 0, 0, 0}, {0, L, 0, 0}, {0, 0, L, 0}, {0, 0, 0, L},
        };
        const int hideAxis = (camera.mode == ViewMode::Slice)
                           ? static_cast<int>(camera.sliceAxis)
                           : -1;
        for (int i = 0; i < 4; ++i) {
            m_axisLabels[i].visible = false;
            if (i == hideAxis) continue;

            const math::Vec4 vw = viewFromWorld.transformPoint(axisLocal[i]);
            const auto p3 = project4to3(vw, camera);
            const float clipX = VP[0]*p3[0] + VP[4]*p3[1] + VP[8] *p3[2] + VP[12];
            const float clipY = VP[1]*p3[0] + VP[5]*p3[1] + VP[9] *p3[2] + VP[13];
            const float clipW = VP[3]*p3[0] + VP[7]*p3[1] + VP[11]*p3[2] + VP[15];
            if (clipW <= 1e-4f) continue;

            const float ndcX = clipX / clipW;
            const float ndcY = clipY / clipW;
            m_axisLabels[i].u = (ndcX + 1.f) * 0.5f * float(m_width);
            m_axisLabels[i].v = (1.f - (ndcY + 1.f) * 0.5f) * float(m_height);
            m_axisLabels[i].visible = true;
        }
    }

    const bool isSolid = camera.style == DisplayStyle::SolidUnlit ||
                         camera.style == DisplayStyle::SolidLit;
    const bool isLit   = camera.style == DisplayStyle::SolidLit;
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    // Collect up to kMaxLights visible directional lights. Each entity's light direction
    // is its rotation applied to the canonical "down" vector, then transformed into view
    // space and projected to 3D so it lives in the same frame as triangle normals.
    constexpr int kMaxLights = 4;
    struct GpuLight { float dx, dy, dz; float intensity; float r, g, b; };
    std::array<GpuLight, kMaxLights> lights{};
    int numLights = 0;
    const math::Vec4 lightCanonical{0.f, -1.f, 0.f, 0.f};
    for (const auto& e : scene.entities()) {
        if (!e.visible || !e.light.has_value()) continue;
        if (numLights >= kMaxLights) break;
        const math::Vec4 d4 = e.transform.rotation.toMatrix().transformDirection(lightCanonical);
        const math::Vec4 dView = viewFromWorld.transformDirection(d4);
        const auto d3 = projectDir4to3(dView, camera);
        // Shader expects direction TO the light source (n . l).
        float lx = -d3[0], ly = -d3[1], lz = -d3[2];
        const float len = std::sqrt(lx * lx + ly * ly + lz * lz);
        if (len > 1e-5f) { lx /= len; ly /= len; lz /= len; }
        lights[numLights++] = {lx, ly, lz,
                               e.light->intensity,
                               e.light->r, e.light->g, e.light->b};
    }

    // Push the light array to the triangle shader once: uniforms are program state so they
    // persist across binds inside the entity loop.
    m_triShader.bind();
    m_triShader.setInt("uNumLights", numLights);
    for (int i = 0; i < numLights; ++i) {
        char name[40];
        std::snprintf(name, sizeof(name), "uLightDir[%d]", i);
        m_triShader.setVec3(name, lights[i].dx, lights[i].dy, lights[i].dz);
        std::snprintf(name, sizeof(name), "uLightIntensity[%d]", i);
        m_triShader.setFloat(name, lights[i].intensity);
        std::snprintf(name, sizeof(name), "uLightColor[%d]", i);
        m_triShader.setVec3(name, lights[i].r, lights[i].g, lights[i].b);
    }

    for (const auto& e : scene.entities()) {
        if (!e.visible || !e.mesh) continue;
        const math::Mat5 viewFromLocal = viewFromWorld * e.transform.toMatrix();

        if (camera.mode == ViewMode::Slice) {
            const SliceMesh sm = sliceMesh(*e.mesh, viewFromLocal,
                                           static_cast<int>(camera.sliceAxis),
                                           camera.sliceVal);
            if (sm.triVertices.empty()) continue;

            glBindVertexArray(m_triVao);
            glBindBuffer(GL_ARRAY_BUFFER, m_triVbo);
            glBufferData(GL_ARRAY_BUFFER,
                         (GLsizeiptr)(sm.triVertices.size() * sizeof(SliceMesh::TriVertex)),
                         sm.triVertices.data(), GL_STREAM_DRAW);

            glPolygonMode(GL_FRONT_AND_BACK, isSolid ? GL_FILL : GL_LINE);

            m_triShader.bind();
            m_triShader.setMat4("uViewProj", VP.data());
            m_triShader.setVec3("uColor", 0.85f, 0.78f, 0.55f);
            m_triShader.setFloat("uAlpha", 1.f);
            m_triShader.setInt("uLit", isLit ? 1 : 0);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)sm.triVertices.size());
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        } else {
            const ProjectedMesh pm = projectMesh(*e.mesh, viewFromLocal,
                                                 camera.projectionStyle == ProjectionStyle::Perspective,
                                                 camera.focal4,
                                                 camera.wOffset);

            if (isSolid && !pm.triVertices.empty()) {
                glBindVertexArray(m_triVao);
                glBindBuffer(GL_ARRAY_BUFFER, m_triVbo);
                glBufferData(GL_ARRAY_BUFFER,
                             (GLsizeiptr)(pm.triVertices.size() * sizeof(ProjectedMesh::TriVertex)),
                             pm.triVertices.data(), GL_STREAM_DRAW);

                m_triShader.bind();
                m_triShader.setMat4("uViewProj", VP.data());
                m_triShader.setVec3("uColor", 0.78f, 0.84f, 0.95f);
                m_triShader.setFloat("uAlpha", 1.f);
                m_triShader.setInt("uLit", isLit ? 1 : 0);
                glDrawArrays(GL_TRIANGLES, 0, (GLsizei)pm.triVertices.size());
            } else if (!isSolid && !pm.lineIndices.empty()) {
                glBindVertexArray(m_lineVao);
                glBindBuffer(GL_ARRAY_BUFFER, m_lineVbo);
                glBufferData(GL_ARRAY_BUFFER,
                             (GLsizeiptr)(pm.linePositions.size() * sizeof(ProjectedMesh::LineVertex)),
                             pm.linePositions.data(), GL_STREAM_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_lineEbo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                             (GLsizeiptr)(pm.lineIndices.size() * sizeof(uint32_t)),
                             pm.lineIndices.data(), GL_STREAM_DRAW);

                m_lineShader.bind();
                m_lineShader.setMat4("uViewProj", VP.data());
                m_lineShader.setVec3("uColor", 1.f, 1.f, 1.f);
                m_lineShader.setInt("uDepthShade",
                                    camera.style == DisplayStyle::DepthWireframe ? 1 : 0);
                m_lineShader.setFloat("uAlpha", 1.f);
                glDrawElements(GL_LINES, (GLsizei)pm.lineIndices.size(),
                               GL_UNSIGNED_INT, nullptr);
            }
        }
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

scene::EntityId ViewportRenderer::pickEntityAt(const scene::Scene& scene,
                                               const Camera4D& camera,
                                               float u, float v) const
{
    if (m_width <= 0 || m_height <= 0) return 0;

    const float yaw   = camera.yaw;
    const float pitch = camera.pitch;
    const float dist  = std::max(0.1f, camera.distance);
    const float dirX = std::cos(pitch) * std::sin(yaw);
    const float dirY = std::sin(pitch);
    const float dirZ = std::cos(pitch) * std::cos(yaw);
    const float ex = camera.pivotX + dist * dirX;
    const float ey = camera.pivotY + dist * dirY;
    const float ez = camera.pivotZ + dist * dirZ;
    const float upY = std::cos(pitch) >= 0.f ? 1.f : -1.f;

    const float aspect  = float(m_width) / float(m_height);
    const float fovYRad = camera.fovYDeg * 3.14159265f / 180.f;
    math::Mat4 P;
    if (camera.projectionStyle == ProjectionStyle::Perspective) {
        P = math::perspective(fovYRad, aspect, camera.zNear, camera.zFar);
    } else {
        const float halfH = std::max(0.05f, dist) * std::tan(fovYRad * 0.5f);
        const float halfW = halfH * aspect;
        P = math::ortho(-halfW, halfW, -halfH, halfH, camera.zNear, camera.zFar);
    }
    const math::Mat4 V = math::lookAt(ex, ey, ez,
                                      camera.pivotX, camera.pivotY, camera.pivotZ,
                                      0.f, upY, 0.f);
    const math::Mat4 VP = math::mat4Mul(P, V);
    const math::Mat5 viewFromWorld = camera.rotation4.toInverseMatrix()
                                   * math::translate4({-camera.position4.x,
                                                       -camera.position4.y,
                                                       -camera.position4.z,
                                                       -camera.position4.w});

    const float ndcX = 2.f * (u / float(m_width)) - 1.f;
    const float ndcY = 1.f - 2.f * (v / float(m_height));

    auto applyVP = [&](float x, float y, float z, float& cx, float& cy, float& cw) {
        cx = VP[0]*x + VP[4]*y + VP[8] *z + VP[12];
        cy = VP[1]*x + VP[5]*y + VP[9] *z + VP[13];
        cw = VP[3]*x + VP[7]*y + VP[11]*z + VP[15];
    };

    auto sign = [](float ax, float ay, float bx, float by, float cx, float cy) {
        return (ax - cx) * (by - cy) - (bx - cx) * (ay - cy);
    };

    scene::EntityId bestId = 0;
    float           bestZ  = 1e30f;

    for (const auto& e : scene.entities()) {
        if (!e.visible || !e.mesh) continue;
        const math::Mat5 viewFromLocal = viewFromWorld * e.transform.toMatrix();

        // Gather projected/sliced triangle vertices.
        std::vector<float> verts; // flat (x,y,z) per vertex, 3 verts per triangle
        if (camera.mode == ViewMode::Slice) {
            const SliceMesh sm = sliceMesh(*e.mesh, viewFromLocal,
                                           static_cast<int>(camera.sliceAxis),
                                           camera.sliceVal);
            verts.reserve(sm.triVertices.size() * 3);
            for (const auto& tv : sm.triVertices) {
                verts.push_back(tv.x); verts.push_back(tv.y); verts.push_back(tv.z);
            }
        } else {
            const ProjectedMesh pm = projectMesh(*e.mesh, viewFromLocal,
                                                 camera.projectionStyle == ProjectionStyle::Perspective,
                                                 camera.focal4, camera.wOffset);
            verts.reserve(pm.triVertices.size() * 3);
            for (const auto& tv : pm.triVertices) {
                verts.push_back(tv.x); verts.push_back(tv.y); verts.push_back(tv.z);
            }
        }

        for (size_t i = 0; i + 8 < verts.size(); i += 9) {
            float ax, ay, aw, bx, by, bw, cx, cy, cw;
            applyVP(verts[i + 0], verts[i + 1], verts[i + 2], ax, ay, aw);
            applyVP(verts[i + 3], verts[i + 4], verts[i + 5], bx, by, bw);
            applyVP(verts[i + 6], verts[i + 7], verts[i + 8], cx, cy, cw);
            if (aw <= 1e-4f || bw <= 1e-4f || cw <= 1e-4f) continue;
            const float ndcAx = ax / aw, ndcAy = ay / aw;
            const float ndcBx = bx / bw, ndcBy = by / bw;
            const float ndcCx = cx / cw, ndcCy = cy / cw;

            const float d1 = sign(ndcX, ndcY, ndcAx, ndcAy, ndcBx, ndcBy);
            const float d2 = sign(ndcX, ndcY, ndcBx, ndcBy, ndcCx, ndcCy);
            const float d3 = sign(ndcX, ndcY, ndcCx, ndcCy, ndcAx, ndcAy);
            const bool hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
            const bool hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);
            if (hasNeg && hasPos) continue;

            // Average NDC z across triangle vertices as a depth estimate.
            const float az = (VP[2]*verts[i+0] + VP[6]*verts[i+1] + VP[10]*verts[i+2] + VP[14]) / aw;
            const float bz = (VP[2]*verts[i+3] + VP[6]*verts[i+4] + VP[10]*verts[i+5] + VP[14]) / bw;
            const float cz = (VP[2]*verts[i+6] + VP[6]*verts[i+7] + VP[10]*verts[i+8] + VP[14]) / cw;
            const float zAvg = (az + bz + cz) * (1.f / 3.f);
            if (zAvg < bestZ) {
                bestZ  = zAvg;
                bestId = e.id;
            }
        }
    }

    return bestId;
}

bool ViewportRenderer::worldToScreen(const Camera4D& camera, const math::Vec4& world,
                                     float& outU, float& outV) const
{
    if (m_width <= 0 || m_height <= 0) return false;

    const float yaw   = camera.yaw;
    const float pitch = camera.pitch;
    const float dist  = std::max(0.1f, camera.distance);
    const float dirX = std::cos(pitch) * std::sin(yaw);
    const float dirY = std::sin(pitch);
    const float dirZ = std::cos(pitch) * std::cos(yaw);
    const float ex = camera.pivotX + dist * dirX;
    const float ey = camera.pivotY + dist * dirY;
    const float ez = camera.pivotZ + dist * dirZ;
    const float upY = std::cos(pitch) >= 0.f ? 1.f : -1.f;

    const float aspect  = float(m_width) / float(m_height);
    const float fovYRad = camera.fovYDeg * 3.14159265f / 180.f;
    math::Mat4 P;
    if (camera.projectionStyle == ProjectionStyle::Perspective) {
        P = math::perspective(fovYRad, aspect, camera.zNear, camera.zFar);
    } else {
        const float halfH = std::max(0.05f, dist) * std::tan(fovYRad * 0.5f);
        const float halfW = halfH * aspect;
        P = math::ortho(-halfW, halfW, -halfH, halfH, camera.zNear, camera.zFar);
    }
    const math::Mat4 V = math::lookAt(ex, ey, ez,
                                      camera.pivotX, camera.pivotY, camera.pivotZ,
                                      0.f, upY, 0.f);
    const math::Mat4 VP = math::mat4Mul(P, V);

    const math::Mat5 viewFromWorld = camera.rotation4.toInverseMatrix()
                                   * math::translate4({-camera.position4.x,
                                                       -camera.position4.y,
                                                       -camera.position4.z,
                                                       -camera.position4.w});

    const math::Vec4 vw = viewFromWorld.transformPoint(world);
    const auto p3 = project4to3(vw, camera);

    const float cx = VP[0]*p3[0] + VP[4]*p3[1] + VP[8] *p3[2] + VP[12];
    const float cy = VP[1]*p3[0] + VP[5]*p3[1] + VP[9] *p3[2] + VP[13];
    const float cw = VP[3]*p3[0] + VP[7]*p3[1] + VP[11]*p3[2] + VP[15];
    if (cw <= 1e-4f) return false;

    const float ndcX = cx / cw;
    const float ndcY = cy / cw;
    outU = (ndcX + 1.f) * 0.5f * float(m_width);
    outV = (1.f - (ndcY + 1.f) * 0.5f) * float(m_height);
    return true;
}

} // namespace hopf::render
