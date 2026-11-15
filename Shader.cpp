#include "Shader.h"
#include <iostream>
#include <fstream>
#include <sstream>

Shader::Shader() : m_programID(0) {
}

Shader::~Shader() {
    cleanup();
}

std::string Shader::readFile(const std::string& filePath) {
    std::ifstream file;
    std::stringstream stream;

    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        file.open(filePath);
        stream << file.rdbuf();
        file.close();
        return stream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << filePath << std::endl;
        std::cerr << "Error: " << e.what() << std::endl;
        return "";
    }
}

GLuint Shader::compileShader(const std::string& source, GLenum shaderType) {
    GLuint shader = glCreateShader(shaderType);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    std::string type = (shaderType == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
    checkCompileErrors(shader, type);

    return shader;
}

GLuint Shader::createProgram(const std::string& vertexShader, const std::string& fragmentShader) {
    GLuint vertexShaderID = compileShader(vertexShader, GL_VERTEX_SHADER);
    GLuint fragmentShaderID = compileShader(fragmentShader, GL_FRAGMENT_SHADER);

    if (vertexShaderID == 0 || fragmentShaderID == 0) {
        if (vertexShaderID) glDeleteShader(vertexShaderID);
        if (fragmentShaderID) glDeleteShader(fragmentShaderID);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShaderID);
    glAttachShader(program, fragmentShaderID);
    glLinkProgram(program);

    checkCompileErrors(program, "PROGRAM");

    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);

    return program;
}

void Shader::checkCompileErrors(GLuint shader, const std::string& type) {
    GLint success;
    GLchar infoLog[1024];

    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << std::endl;
            std::cerr << infoLog << std::endl;
            std::cerr << "-- --------------------------------------------------- --" << std::endl;
        }
    }
    else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << std::endl;
            std::cerr << infoLog << std::endl;
            std::cerr << "-- --------------------------------------------------- --" << std::endl;
        }
    }
}

GLint Shader::getUniformLocation(const std::string& name) {
    auto it = m_uniformLocationCache.find(name);
    if (it != m_uniformLocationCache.end()) {
        return it->second;
    }

    GLint location = glGetUniformLocation(m_programID, name.c_str());
    if (location == -1) {
        std::cerr << "Warning: uniform '" << name << "' doesn't exist!" << std::endl;
    }

    m_uniformLocationCache[name] = location;
    return location;
}

bool Shader::loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexCode = readFile(vertexPath);
    std::string fragmentCode = readFile(fragmentPath);

    if (vertexCode.empty() || fragmentCode.empty()) {
        std::cerr << "Failed to read shader files" << std::endl;
        return false;
    }

    return loadFromSource(vertexCode, fragmentCode);
}

bool Shader::loadFromSource(const std::string& vertexSource, const std::string& fragmentSource) {
    cleanup();

    m_programID = createProgram(vertexSource, fragmentSource);

    if (m_programID == 0) {
        std::cerr << "Failed to create shader program" << std::endl;
        return false;
    }

    std::cout << "Shader program created successfully (ID: " << m_programID << ")" << std::endl;
    return true;
}

void Shader::use() const {
    if (m_programID != 0) {
        glUseProgram(m_programID);
    }
}

void Shader::unuse() const {
    glUseProgram(0);
}

void Shader::setBool(const std::string& name, bool value) {
    glUniform1i(getUniformLocation(name), static_cast<int>(value));
}

void Shader::setInt(const std::string& name, int value) {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value) {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) {
    glUniform2fv(getUniformLocation(name), 1, &value[0]);
}

void Shader::setVec2(const std::string& name, float x, float y) {
    glUniform2f(getUniformLocation(name), x, y);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) {
    glUniform3fv(getUniformLocation(name), 1, &value[0]);
}

void Shader::setVec3(const std::string& name, float x, float y, float z) {
    glUniform3f(getUniformLocation(name), x, y, z);
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) {
    glUniform4fv(getUniformLocation(name), 1, &value[0]);
}

void Shader::setVec4(const std::string& name, float x, float y, float z, float w) {
    glUniform4f(getUniformLocation(name), x, y, z, w);
}

void Shader::setMat2(const std::string& name, const glm::mat2& mat) {
    glUniformMatrix2fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

void Shader::setMat3(const std::string& name, const glm::mat3& mat) {
    glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

void Shader::cleanup() {
    if (m_programID != 0) {
        glDeleteProgram(m_programID);
        m_programID = 0;
        m_uniformLocationCache.clear();
    }
}