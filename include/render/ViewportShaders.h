#pragma once

namespace hopf::render::shaders {

inline constexpr const char* kLineVS = R"GLSL(
#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in float aWDepth;

uniform mat4 uViewProj;
out float vW;
void main() {
    vW = aWDepth;
    gl_Position = uViewProj * vec4(aPos, 1.0);
}
)GLSL";

inline constexpr const char* kLineFS = R"GLSL(
#version 460 core
in float vW;
uniform vec3  uColor;
uniform int   uDepthShade;
uniform float uAlpha;
out vec4 oColor;
void main() {
    if (uDepthShade == 1) {
        float t = clamp(0.5 + 0.4 * vW, 0.0, 1.0);
        vec3 cool = vec3(0.30, 0.55, 1.00);
        vec3 warm = vec3(1.00, 0.65, 0.30);
        oColor = vec4(mix(cool, warm, t) * uColor, uAlpha);
    } else {
        oColor = vec4(uColor, uAlpha);
    }
}
)GLSL";

inline constexpr const char* kTriVS = R"GLSL(
#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
uniform mat4 uViewProj;
flat out vec3 vNormal;
void main() {
    vNormal = aNormal;
    gl_Position = uViewProj * vec4(aPos, 1.0);
}
)GLSL";

inline constexpr const char* kTriFS = R"GLSL(
#version 460 core
flat in vec3 vNormal;
uniform vec3  uColor;
uniform float uAlpha;
uniform int   uLit;
const int kMaxLights = 4;
uniform int   uNumLights;
uniform vec3  uLightDir[kMaxLights];       // unit vector toward each light, render space
uniform float uLightIntensity[kMaxLights]; // 1.0 = nominal
uniform vec3  uLightColor[kMaxLights];     // tint per light
out vec4 oColor;
void main() {
    vec3 c = uColor;
    if (uLit == 1) {
        vec3 n   = normalize(vNormal);
        vec3 lit = vec3(0.15); // ambient floor so unlit faces aren't pitch black
        for (int i = 0; i < uNumLights; ++i) {
            float ndl = max(0.0, dot(n, uLightDir[i])) * uLightIntensity[i];
            lit += ndl * uLightColor[i];
        }
        c *= lit;
    }
    oColor = vec4(c, uAlpha);
}
)GLSL";

} // namespace hopf::render::shaders
