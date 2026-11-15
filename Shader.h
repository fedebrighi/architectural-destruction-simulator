#pragma once

#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader {
private:
    GLuint m_programID;
    std::unordered_map<std::string, GLint> m_uniformLocationCache;

    std::string readFile(const std::string& filePath);
    GLuint compileShader(const std::string& source, GLenum shaderType);
    GLuint createProgram(const std::string& vertexShader, const std::string& fragmentShader);
    void checkCompileErrors(GLuint shader, const std::string& type);

    GLint getUniformLocation(const std::string& name);

public:
    Shader();
    ~Shader();

    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);

    bool loadFromSource(const std::string& vertexSource, const std::string& fragmentSource);

    void use() const;
    void unuse() const;

    void setBool(const std::string& name, bool value);
    void setInt(const std::string& name, int value);
    void setFloat(const std::string& name, float value);
    void setVec2(const std::string& name, const glm::vec2& value);
    void setVec2(const std::string& name, float x, float y);
    void setVec3(const std::string& name, const glm::vec3& value);
    void setVec3(const std::string& name, float x, float y, float z);
    void setVec4(const std::string& name, const glm::vec4& value);
    void setVec4(const std::string& name, float x, float y, float z, float w);
    void setMat2(const std::string& name, const glm::mat2& mat);
    void setMat3(const std::string& name, const glm::mat3& mat);
    void setMat4(const std::string& name, const glm::mat4& mat);

    GLuint getProgramID() const { return m_programID; }
    bool isValid() const { return m_programID != 0; }

    void cleanup();
};