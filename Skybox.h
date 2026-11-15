#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"
#include <string>
#include <vector>
#include <memory>

class Skybox {
private:
    GLuint m_VAO, m_VBO;
    GLuint m_cubemapTexture;
    std::unique_ptr<Shader> m_skyboxShader;

    void setupSkyboxGeometry();
    void createSkyboxShader();
    GLuint loadCubemap(const std::vector<std::string>& faces);

public:
    Skybox();
    ~Skybox();

    bool initialize(const std::vector<std::string>& faces);
    void render(const glm::mat4& view, const glm::mat4& projection);
    void cleanup();

    bool isValid() const { return m_cubemapTexture != 0 && m_skyboxShader && m_skyboxShader->isValid(); }
};