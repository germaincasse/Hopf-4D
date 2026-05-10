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
uniform vec3 uColor;
uniform int  uDepthShade;
out vec4 oColor;
void main() {
    if (uDepthShade == 1) {
        float t = clamp(0.5 + 0.4 * vW, 0.0, 1.0);
        vec3 cool = vec3(0.30, 0.55, 1.00);
        vec3 warm = vec3(1.00, 0.65, 0.30);
        oColor = vec4(mix(cool, warm, t) * uColor, 1.0);
    } else {
        oColor = vec4(uColor, 1.0);
    }
}
)GLSL";

inline constexpr const char* kTriVS = R"GLSL(
#version 460 core
layout(location = 0) in vec3 aPos;
uniform mat4 uViewProj;
out vec3 vPosWorld;
void main() {
    vPosWorld = aPos;
    gl_Position = uViewProj * vec4(aPos, 1.0);
}
)GLSL";

inline constexpr const char* kTriFS = R"GLSL(
#version 460 core
in vec3 vPosWorld;
uniform vec3  uColor;
uniform float uAlpha;
uniform int   uLit;
out vec4 oColor;
void main() {
    vec3 c = uColor;
    if (uLit == 1) {
        vec3 n = normalize(cross(dFdx(vPosWorld), dFdy(vPosWorld)));
        float ndl = max(0.2, abs(dot(n, normalize(vec3(0.4, 0.7, 0.5)))));
        c *= ndl;
    }
    oColor = vec4(c, uAlpha);
}
)GLSL";

} // namespace hopf::render::shaders
