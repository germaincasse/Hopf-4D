#include "render/Shader.h"

#include "core/Logger.h"

#include <glad/gl.h>

#include <utility>
#include <vector>

namespace hopf::render {

namespace {

uint32_t compileStage(uint32_t kind, const char* src) {
    const uint32_t shader = glCreateShader(kind);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(static_cast<size_t>(len > 0 ? len : 1));
        glGetShaderInfoLog(shader, len, nullptr, log.data());
        hopf::core::Logger::error("Shader compile failed (%s): %s",
            kind == GL_VERTEX_SHADER ? "vertex" : "fragment",
            log.data());
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

} // namespace

Shader::~Shader() {
    if (m_program) glDeleteProgram(m_program);
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_program) glDeleteProgram(m_program);
        m_program = other.m_program;
        other.m_program = 0;
    }
    return *this;
}

bool Shader::compile(const char* vertexSrc, const char* fragmentSrc) {
    const uint32_t vs = compileStage(GL_VERTEX_SHADER,   vertexSrc);
    if (!vs) return false;
    const uint32_t fs = compileStage(GL_FRAGMENT_SHADER, fragmentSrc);
    if (!fs) { glDeleteShader(vs); return false; }

    const uint32_t prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(static_cast<size_t>(len > 0 ? len : 1));
        glGetProgramInfoLog(prog, len, nullptr, log.data());
        hopf::core::Logger::error("Shader link failed: %s", log.data());
        glDeleteProgram(prog);
        return false;
    }

    if (m_program) glDeleteProgram(m_program);
    m_program = prog;
    return true;
}

void Shader::bind() const   { glUseProgram(m_program); }
void Shader::unbind() const { glUseProgram(0); }

void Shader::setMat4(const char* name, const float* m) const {
    const int loc = glGetUniformLocation(m_program, name);
    if (loc >= 0) glUniformMatrix4fv(loc, 1, GL_FALSE, m);
}

void Shader::setVec3(const char* name, float x, float y, float z) const {
    const int loc = glGetUniformLocation(m_program, name);
    if (loc >= 0) glUniform3f(loc, x, y, z);
}

void Shader::setFloat(const char* name, float v) const {
    const int loc = glGetUniformLocation(m_program, name);
    if (loc >= 0) glUniform1f(loc, v);
}

void Shader::setInt(const char* name, int v) const {
    const int loc = glGetUniformLocation(m_program, name);
    if (loc >= 0) glUniform1i(loc, v);
}

} // namespace hopf::render
