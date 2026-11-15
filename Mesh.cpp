#include "Mesh.h"
#include <iostream>

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices, std::vector<Texture> textures)
    : vertices(vertices), indices(indices), textures(textures), m_VAO(0), m_VBO(0), m_EBO(0) {
    setupMesh();
}

Mesh::~Mesh() {
    cleanup();
}

void Mesh::setupMesh() {
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));

    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

    // TexCoords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

    // Tangent → location 3
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));

    // Bitangent → location 4
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

    // Bone IDs → location 5
    glEnableVertexAttribArray(5);
    glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, BoneIDs));

    // Bone Weights → location 6
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Weights));

    glBindVertexArray(0);
}



void Mesh::render(Shader& shader) {
    // Contatori per ogni tipo di texture
    GLuint diffuseNr = 1;
    GLuint specularNr = 1;
    GLuint normalNr = 1;
    GLuint heightNr = 1;

    bool hasAnyTexture = !textures.empty();
    shader.setBool("useTexture", hasAnyTexture);

    for (GLuint i = 0; i < textures.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + i);

        std::string number;
        std::string name = textures[i].type;

        if (name == "texture_diffuse")
            number = std::to_string(diffuseNr++);
        else if (name == "texture_specular")
            number = std::to_string(specularNr++);
        else if (name == "texture_normal")
            number = std::to_string(normalNr++);
        else if (name == "texture_height")
            number = std::to_string(heightNr++);

        std::string uniformName = name + number;

        GLint location = glGetUniformLocation(shader.getProgramID(), uniformName.c_str());
        if (location != -1) {
            shader.setInt(uniformName.c_str(), i);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);

        }
    }

    // Disegna la mesh
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLuint>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Reset dello stato OpenGL
    glActiveTexture(GL_TEXTURE0);
}

void Mesh::cleanup() {
    if (m_VAO) {
        glDeleteVertexArrays(1, &m_VAO);
        glDeleteBuffers(1, &m_VBO);
        glDeleteBuffers(1, &m_EBO);
        m_VAO = m_VBO = m_EBO = 0;
    }
}

glm::vec3 Mesh::calculateCenter() const {
    if (vertices.empty()) return glm::vec3(0.0f);

    glm::vec3 center(0.0f);
    for (const auto& vertex : vertices) {
        center += vertex.Position;
    }

    return center / static_cast<float>(vertices.size());
}

float Mesh::calculateBoundingRadius() const {
    if (vertices.empty()) return 0.0f;

    glm::vec3 center = calculateCenter();
    float maxDistance = 0.0f;

    for (const auto& vertex : vertices) {
        float distance = glm::length(vertex.Position - center);
        maxDistance = std::max(maxDistance, distance);
    }

    return maxDistance;
}

glm::vec3 Mesh::calculateBoundingBox() const {
    if (vertices.empty()) return glm::vec3(0.0f);

    glm::vec3 minPos = vertices[0].Position;
    glm::vec3 maxPos = vertices[0].Position;

    for (const auto& vertex : vertices) {
        minPos = glm::min(minPos, vertex.Position);
        maxPos = glm::max(maxPos, vertex.Position);
    }

    return maxPos - minPos; 
}

void Mesh::updateVertexData() {
    // Bind del Vertex Array Object
    glBindVertexArray(m_VAO);

    // Bind del Vertex Buffer Object
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), vertices.data());

    // Unbind per sicurezza
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    std::cout << "Updated vertex data for mesh with " << vertices.size() << " vertices" << std::endl;
}