#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"
#include "Camera.h"
#include <memory>

class Renderer {
private:
    GLuint m_quadVAO, m_quadVBO;
    GLuint m_cubeVAO, m_cubeVBO;
    GLuint m_groundVAO, m_groundVBO;

    std::unique_ptr<Shader> m_basicShader;
    std::unique_ptr<Shader> m_instancedShader;

    void setupQuad();
    void setupCube();
    void createEnhancedShader();

public:
    Renderer();
    ~Renderer();

    bool initialize();
    void cleanup();
    
    Shader* getInstancedShader() const { return m_instancedShader.get(); }
    bool loadInstancedShader(const std::string& vertexPath, const std::string& fragmentPath);

    void clear(const glm::vec3& color = glm::vec3(0.2f, 0.3f, 0.3f));
    void setViewport(int width, int height);

    void renderQuad(const glm::mat4& mvp, const glm::vec3& color = glm::vec3(1.0f));
    void renderCube(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection,
        const glm::vec3& color = glm::vec3(1.0f));

    void beginScene(Camera* camera, int windowWidth, int windowHeight, float deltaTime = 0.016f);
    void endScene();

    Shader* getBasicShader() const { return m_basicShader.get(); }

    void renderFragment(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection,
    float explosionFactor = 1.0f, float time = 0.0f);
    void renderIntactBuilding(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection);

    void setupGroundPlane();
    void renderGroundPlane(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& color = glm::vec3(0.85f, 0.82f, 0.78f));

};