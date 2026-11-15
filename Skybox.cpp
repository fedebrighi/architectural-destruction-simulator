#include "Skybox.h"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Skybox::Skybox() : m_VAO(0), m_VBO(0), m_cubemapTexture(0) {
}

Skybox::~Skybox() {
    cleanup();
}

void Skybox::setupSkyboxGeometry() {
    float skyboxVertices[] = {     
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Skybox::createSkyboxShader() {
    m_skyboxShader = std::make_unique<Shader>();

    if (!m_skyboxShader->loadFromFiles("skybox_vertex.glsl", "skybox_fragment.glsl")) {
        std::cerr << "Failed to load skybox shaders from files" << std::endl;
        m_skyboxShader.reset();
    }
}

GLuint Skybox::loadCubemap(const std::vector<std::string>& faces) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format = GL_RGB;
            if (nrChannels == 1) format = GL_RED;
            else if (nrChannels == 3) format = GL_RGB;
            else if (nrChannels == 4) format = GL_RGBA;

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);

            std::cout << "Loaded skybox face " << i << ": " << faces[i] << " (" << width << "x" << height << ")" << std::endl;
        }
        else {
            std::cerr << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
            glDeleteTextures(1, &textureID);
            return 0;
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    return textureID;
}

bool Skybox::initialize(const std::vector<std::string>& faces) {
    if (faces.size() != 6) {
        std::cerr << "Skybox requires exactly 6 faces" << std::endl;
        return false;
    }

    std::cout << "Initializing skybox..." << std::endl;

    setupSkyboxGeometry();

    createSkyboxShader();
    if (!m_skyboxShader || !m_skyboxShader->isValid()) {
        std::cerr << "Failed to create skybox shader" << std::endl;
        return false;
    }

    m_cubemapTexture = loadCubemap(faces);
    if (m_cubemapTexture == 0) {
        std::cerr << "Failed to load skybox cubemap" << std::endl;
        return false;
    }

    std::cout << "Skybox initialized successfully" << std::endl;
    return true;
}

void Skybox::render(const glm::mat4& view, const glm::mat4& projection) {
    if (!isValid()) return;

    glDepthFunc(GL_LEQUAL);

    m_skyboxShader->use();

    glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));

    m_skyboxShader->setMat4("view", viewNoTranslation);
    m_skyboxShader->setMat4("projection", projection);
    m_skyboxShader->setInt("skybox", 0);

    glBindVertexArray(m_VAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);
}

void Skybox::cleanup() {
    if (m_VAO) {
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        m_VAO = m_VBO = 0;
    }

    if (m_cubemapTexture) {
        glDeleteTextures(1, &m_cubemapTexture);
        m_cubemapTexture = 0;
    }

    m_skyboxShader.reset();
}