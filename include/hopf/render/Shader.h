#pragma once

#include <cstdint>
#include <string>

namespace hopf::render {

class Shader {
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept { *this = std::move(other); }
    Shader& operator=(Shader&& other) noexcept;

    bool compile(const char* vertexSrc, const char* fragmentSrc);

    void bind() const;
    void unbind() const;

    void setMat4(const char* name, const float* m4x4ColumnMajor) const;
    void setVec3(const char* name, float x, float y, float z) const;
    void setFloat(const char* name, float v) const;
    void setInt(const char* name, int v) const;

    uint32_t handle() const { return m_program; }

private:
    uint32_t m_program = 0;
};

} // namespace hopf::render
