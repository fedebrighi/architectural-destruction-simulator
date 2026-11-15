#include "Renderer.h"
#include <iostream>

Renderer::Renderer() : m_quadVAO(0), m_quadVBO(0), m_cubeVAO(0), m_cubeVBO(0), m_groundVAO(0), m_groundVBO(0) {
}

Renderer::~Renderer() {
    cleanup();
}

bool Renderer::initialize() {
    std::cout << "Initializing Enhanced Renderer..." << std::endl;

    setupQuad();
    setupCube();
    setupGroundPlane();

    createEnhancedShader();

    if (!m_basicShader || !m_basicShader->isValid()) {
        std::cerr << "Failed to create enhanced shader" << std::endl;
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::cout << "Enhanced Renderer initialized successfully" << std::endl;
    return true;
}


void Renderer::cleanup() {
    if (m_quadVAO) {
        glDeleteVertexArrays(1, &m_quadVAO);
        glDeleteBuffers(1, &m_quadVBO);
        m_quadVAO = m_quadVBO = 0;
    }

    if (m_cubeVAO) {
        glDeleteVertexArrays(1, &m_cubeVAO);
        glDeleteBuffers(1, &m_cubeVBO);
        m_cubeVAO = m_cubeVBO = 0;
    }


    if (m_groundVAO) {
        glDeleteVertexArrays(1, &m_groundVAO);
        glDeleteBuffers(1, &m_groundVBO);
        m_groundVAO = m_groundVBO = 0;
    }

    m_basicShader.reset();
}



void Renderer::setupGroundPlane() {
    float groundVertices[] = {
        -25.0f, 0.0f, -25.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,  
         25.0f, 0.0f,  25.0f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,  
         25.0f, 0.0f, -25.0f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,  

         -25.0f, 0.0f, -25.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,  
         -25.0f, 0.0f,  25.0f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,  
          25.0f, 0.0f,  25.0f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f  
    };

    glGenVertexArrays(1, &m_groundVAO);
    glGenBuffers(1, &m_groundVBO);

    glBindVertexArray(m_groundVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_groundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), groundVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Renderer::renderGroundPlane(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& color) {
    if (!m_basicShader || !m_basicShader->isValid()) return;

    m_basicShader->use();

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));

    m_basicShader->setMat4("model", model);
    m_basicShader->setMat4("view", view);
    m_basicShader->setMat4("projection", projection);

    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
    m_basicShader->setMat3("normalMatrix", normalMatrix);

    m_basicShader->setVec3("objectColor", color);
    m_basicShader->setBool("isFragment", false);
    m_basicShader->setFloat("explosionFactor", 0.0f);

    glBindVertexArray(m_groundVAO);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
}

void Renderer::setupQuad() {
    float quadVertices[] = {
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f
    };

    GLuint indices[] = {
        0, 1, 2,  
        2, 3, 0   
    };

    GLuint EBO;
    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(m_quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Renderer::setupCube() {
    float cubeVertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

         -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
          0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
          0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
          0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
         -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

         -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
          0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
          0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
          0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
         -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
    };

    glGenVertexArrays(1, &m_cubeVAO);
    glGenBuffers(1, &m_cubeVBO);

    glBindVertexArray(m_cubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Renderer::createEnhancedShader() {
    std::cout << "Loading shaders from files..." << std::endl;

    m_basicShader = std::make_unique<Shader>();

    if (!m_basicShader->loadFromFiles("vertex.glsl", "fragment.glsl")) {
        std::cerr << "Failed to load shaders from files!" << std::endl;
        std::cerr << "Make sure vertex.glsl and fragment.glsl are in the project directory." << std::endl;
        return;
    }


    std::cout << "Shaders loaded successfully from files!" << std::endl;
}

void Renderer::clear(const glm::vec3& color) {
    glClearColor(color.r, color.g, color.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::setViewport(int width, int height) {
    glViewport(0, 0, width, height);
}

void Renderer::renderQuad(const glm::mat4& mvp, const glm::vec3& color) {
    if (!m_basicShader || !m_basicShader->isValid()) return;

    m_basicShader->use();
    m_basicShader->setMat4("model", mvp);
    m_basicShader->setVec3("objectColor", color);
    m_basicShader->setBool("isFragment", false);

    glBindVertexArray(m_quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Renderer::renderCube(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& color) {
    if (!m_basicShader || !m_basicShader->isValid()) return;

    m_basicShader->use();
    m_basicShader->setMat4("model", model);
    m_basicShader->setMat4("view", view);
    m_basicShader->setMat4("projection", projection);
    m_basicShader->setBool("useObjectColor", true);

    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
    m_basicShader->setMat3("normalMatrix", normalMatrix);

    m_basicShader->setVec3("objectColor", color);
    m_basicShader->setBool("isFragment", false);


    glBindVertexArray(m_cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void Renderer::renderFragment(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection,
    float explosionFactor, float time) {
    if (!m_basicShader || !m_basicShader->isValid()) return;

    m_basicShader->use();
    m_basicShader->setMat4("model", model);
    m_basicShader->setMat4("view", view);
    m_basicShader->setMat4("projection", projection);

    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
    m_basicShader->setMat3("normalMatrix", normalMatrix);

    m_basicShader->setVec3("objectColor", glm::vec3(0.8f, 0.2f, 0.1f));
    m_basicShader->setBool("isFragment", true);
    m_basicShader->setFloat("explosionFactor", explosionFactor);
    m_basicShader->setFloat("time", time);

    glBindVertexArray(m_cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void Renderer::renderIntactBuilding(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection) {
    if (!m_basicShader || !m_basicShader->isValid()) return;

    m_basicShader->use();
    m_basicShader->setMat4("model", model);
    m_basicShader->setMat4("view", view);
    m_basicShader->setMat4("projection", projection);

    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
    m_basicShader->setMat3("normalMatrix", normalMatrix);

    m_basicShader->setVec3("objectColor", glm::vec3(0.6f, 0.6f, 0.65f)); 
    m_basicShader->setBool("isFragment", false);
    m_basicShader->setFloat("explosionFactor", 0.0f);

    glBindVertexArray(m_cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void Renderer::beginScene(Camera* camera, int windowWidth, int windowHeight, float deltaTime) {
    if (!camera) return;
    setViewport(windowWidth, windowHeight);

    if (m_basicShader && m_basicShader->isValid()) {
        m_basicShader->use();

        static float globalTime = 0.0f;
        globalTime += deltaTime;
        m_basicShader->setFloat("time", globalTime);

        m_basicShader->setVec3("lightColor", 1.0f, 1.0f, 1.0f);
        m_basicShader->setVec3("viewPos", camera->getPosition());
        m_basicShader->setVec3("lightPos", 50.0f, 50.0f, 30.0f);
        m_basicShader->setFloat("ambientStrength", 0.25f);
        m_basicShader->setFloat("specularStrength", 0.6f);
        m_basicShader->setInt("shininess", 32);

        m_basicShader->setBool("useTexture", false);
        m_basicShader->setBool("useObjectColor", false);
        m_basicShader->setBool("isFragment", false);
        m_basicShader->setFloat("explosionFactor", 0.0f);
        m_basicShader->setVec3("objectColor", 0.65f, 0.65f, 0.65f);
    }
}

bool Renderer::loadInstancedShader(const std::string& vertexPath, const std::string& fragmentPath) {
    try {
        m_instancedShader = std::make_unique<Shader>();

        if (!m_instancedShader->loadFromFiles(vertexPath, fragmentPath)) {
            std::cerr << "Failed to load instanced shader files" << std::endl;
            m_instancedShader.reset();
            return false;
        }

        std::cout << "Instanced shader loaded successfully" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception loading instanced shader: " << e.what() << std::endl;
        m_instancedShader.reset();
        return false;
    }
}

void Renderer::endScene() {
    glUseProgram(0);
}