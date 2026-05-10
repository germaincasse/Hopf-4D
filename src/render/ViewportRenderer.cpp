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

    // The W axis collapses to a point in Slice mode (it's normal to the hyperplane),
    // so we skip it there.
    const int axisCount = (camera.mode == ViewMode::Projection) ? 4 : 3;

    auto project4to3 = [&](const math::Vec4& v) -> std::array<float, 3> {
        if (camera.mode == ViewMode::Projection && camera.perspective4) {
            const float wp = v.w + camera.wOffset;
            const float k = wp != 0.f ? camera.focal4 / wp : 1.f;
            return {v.x * k, v.y * k, v.z * k};
        }
        return {v.x, v.y, v.z};
    };

    const math::Vec4 originView = viewFromWorld.transformPoint({0, 0, 0, 0});
    const auto       originProj = project4to3(originView);

    struct V { float x, y, z, w; };
    V        verts[8];
    uint32_t idx[8];
    for (int i = 0; i < axisCount; ++i) {
        const math::Vec4 endView = viewFromWorld.transformPoint(axisLocal[i]);
        const auto       endProj = project4to3(endView);
        verts[i * 2 + 0] = {originProj[0], originProj[1], originProj[2], originView.w};
        verts[i * 2 + 1] = {endProj[0],    endProj[1],    endProj[2],    endView.w};
        idx[i * 2 + 0] = static_cast<uint32_t>(i * 2 + 0);
        idx[i * 2 + 1] = static_cast<uint32_t>(i * 2 + 1);
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(axisCount * 2 * sizeof(V)),
                 verts, GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(axisCount * 2 * sizeof(uint32_t)),
                 idx, GL_STREAM_DRAW);

    s.bind();
    s.setInt("uDepthShade", 0);
    for (int i = 0; i < axisCount; ++i) {
        s.setVec3("uColor", colors[i][0], colors[i][1], colors[i][2]);
        glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT,
                       (void*)(static_cast<intptr_t>(i * 2 * sizeof(uint32_t))));
    }
}

} // namespace

ViewportRenderer::ViewportRenderer() = default;

ViewportRenderer::~ViewportRenderer() {
    destroyFramebuffer();

    if (m_lineEbo) glDeleteBuffers(1, &m_lineEbo);
    if (m_lineVbo) glDeleteBuffers(1, &m_lineVbo);
    if (m_lineVao) glDeleteVertexArrays(1, &m_lineVao);

    if (m_triEbo) glDeleteBuffers(1, &m_triEbo);
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
        glGenBuffers(1, &m_triEbo);

        glBindVertexArray(m_triVao);
        glBindBuffer(GL_ARRAY_BUFFER, m_triVbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_triEbo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(0));
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

    const float aspect = float(m_width) / float(m_height);
    const math::Mat4 P = math::perspective(camera.fovYDeg * 3.14159265f / 180.f,
                                           aspect, camera.zNear, camera.zFar);
    const math::Mat4 V = math::lookAt(ex, ey, ez,
                                      camera.pivotX, camera.pivotY, camera.pivotZ,
                                      0.f, upY, 0.f);
    const math::Mat4 VP = math::mat4Mul(P, V);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);
    glClearColor(0.10f, 0.11f, 0.13f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(1.5f);
    // 4D projection produces both-facing triangles; never cull.
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    const math::Mat5 viewFromWorld = camera.rotation4.toInverseMatrix();

    m_lineShader.bind();
    m_lineShader.setMat4("uViewProj", VP.data());
    drawWorldAxes(m_lineVao, m_lineVbo, m_lineEbo, m_lineShader, viewFromWorld, camera);

    {
        // Cache the screen-space tip of each gizmo axis so the editor can overlay letters.
        const float L = 1.f;
        const math::Vec4 axisLocal[4] = {
            {L, 0, 0, 0}, {0, L, 0, 0}, {0, 0, L, 0}, {0, 0, 0, L},
        };
        const int axisCount = (camera.mode == ViewMode::Projection) ? 4 : 3;
        for (int i = 0; i < 4; ++i) {
            m_axisLabels[i].visible = false;
            if (i >= axisCount) continue;

            const math::Vec4 vw = viewFromWorld.transformPoint(axisLocal[i]);
            float p3x = vw.x, p3y = vw.y, p3z = vw.z;
            if (camera.mode == ViewMode::Projection && camera.perspective4) {
                const float wp = vw.w + camera.wOffset;
                const float k  = wp != 0.f ? camera.focal4 / wp : 1.f;
                p3x *= k; p3y *= k; p3z *= k;
            }
            const float clipX = VP[0]*p3x + VP[4]*p3y + VP[8] *p3z + VP[12];
            const float clipY = VP[1]*p3x + VP[5]*p3y + VP[9] *p3z + VP[13];
            const float clipW = VP[3]*p3x + VP[7]*p3y + VP[11]*p3z + VP[15];
            if (clipW <= 1e-4f) continue;

            const float ndcX = clipX / clipW;
            const float ndcY = clipY / clipW;
            m_axisLabels[i].u = (ndcX + 1.f) * 0.5f * float(m_width);
            // Flip Y: GL's NDC has +Y up, ImGui's image coords have +Y down.
            m_axisLabels[i].v = (1.f - (ndcY + 1.f) * 0.5f) * float(m_height);
            m_axisLabels[i].visible = true;
        }
    }

    const bool isSolid = camera.style == DisplayStyle::SolidUnlit ||
                         camera.style == DisplayStyle::SolidLit;
    const bool isLit   = camera.style == DisplayStyle::SolidLit;
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    for (const auto& e : scene.entities()) {
        if (!e.visible || !e.mesh) continue;
        const math::Mat5 viewFromLocal = viewFromWorld * e.transform.toMatrix();

        if (camera.mode == ViewMode::Slice) {
            const SliceMesh sm = sliceMesh(*e.mesh, viewFromLocal, camera.sliceW);
            if (sm.indices.empty()) continue;

            glBindVertexArray(m_triVao);
            glBindBuffer(GL_ARRAY_BUFFER, m_triVbo);
            glBufferData(GL_ARRAY_BUFFER,
                         (GLsizeiptr)(sm.positions.size() * sizeof(SliceMesh::Vertex3)),
                         sm.positions.data(), GL_STREAM_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_triEbo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         (GLsizeiptr)(sm.indices.size() * sizeof(uint32_t)),
                         sm.indices.data(), GL_STREAM_DRAW);

            glPolygonMode(GL_FRONT_AND_BACK, isSolid ? GL_FILL : GL_LINE);

            m_triShader.bind();
            m_triShader.setMat4("uViewProj", VP.data());
            m_triShader.setVec3("uColor", 0.85f, 0.78f, 0.55f);
            m_triShader.setFloat("uAlpha", 1.f);
            m_triShader.setInt("uLit", isLit ? 1 : 0);
            glDrawElements(GL_TRIANGLES, (GLsizei)sm.indices.size(), GL_UNSIGNED_INT, nullptr);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        } else {
            const ProjectedMesh pm = projectMesh(*e.mesh, viewFromLocal,
                                                 camera.perspective4,
                                                 camera.focal4,
                                                 camera.wOffset);

            if (isSolid && !pm.triangleIndices.empty()) {
                // The line VBO uses (xyz, wDepth) stride 16; the tri shader only reads
                // location 0 (vec3), so the wDepth lane is ignored.
                glBindVertexArray(m_lineVao);
                glBindBuffer(GL_ARRAY_BUFFER, m_lineVbo);
                glBufferData(GL_ARRAY_BUFFER,
                             (GLsizeiptr)(pm.positions.size() * sizeof(ProjectedMesh::Vertex3)),
                             pm.positions.data(), GL_STREAM_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_lineEbo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                             (GLsizeiptr)(pm.triangleIndices.size() * sizeof(uint32_t)),
                             pm.triangleIndices.data(), GL_STREAM_DRAW);

                m_triShader.bind();
                m_triShader.setMat4("uViewProj", VP.data());
                m_triShader.setVec3("uColor", 0.78f, 0.84f, 0.95f);
                m_triShader.setFloat("uAlpha", 1.f);
                m_triShader.setInt("uLit", isLit ? 1 : 0);
                glDrawElements(GL_TRIANGLES, (GLsizei)pm.triangleIndices.size(),
                               GL_UNSIGNED_INT, nullptr);
            } else if (!isSolid && !pm.lineIndices.empty()) {
                glBindVertexArray(m_lineVao);
                glBindBuffer(GL_ARRAY_BUFFER, m_lineVbo);
                glBufferData(GL_ARRAY_BUFFER,
                             (GLsizeiptr)(pm.positions.size() * sizeof(ProjectedMesh::Vertex3)),
                             pm.positions.data(), GL_STREAM_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_lineEbo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                             (GLsizeiptr)(pm.lineIndices.size() * sizeof(uint32_t)),
                             pm.lineIndices.data(), GL_STREAM_DRAW);

                m_lineShader.bind();
                m_lineShader.setMat4("uViewProj", VP.data());
                m_lineShader.setVec3("uColor", 1.f, 1.f, 1.f);
                m_lineShader.setInt("uDepthShade",
                                    camera.style == DisplayStyle::DepthWireframe ? 1 : 0);
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

} // namespace hopf::render
