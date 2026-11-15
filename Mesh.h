#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"
#include <vector>
#include <string>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
    glm::ivec4 BoneIDs = glm::ivec4(-1);
    glm::vec4 Weights = glm::vec4(0.0f);
};

struct Texture {
    GLuint id;
    std::string type;
    std::string path;
};

class Mesh {
private:
    GLuint m_VAO, m_VBO, m_EBO;
    std::vector<unsigned int> m_indices;
    void setupMesh();

public:
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Texture> textures;

    Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices, std::vector<Texture> textures);
    ~Mesh();

    void render(Shader& shader);
    void cleanup();

    const std::vector<Vertex>& getVertices() const { return vertices; }
    const std::vector<GLuint>& getIndices() const { return indices; }

    void updateVertexData();

    glm::vec3 calculateCenter() const;
    float calculateBoundingRadius() const;
    glm::vec3 calculateBoundingBox() const;

    GLuint getVAO() const { return m_VAO; }
    GLsizei getIndexCount() const { return static_cast<GLsizei>(m_indices.size()); }
};