#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include "Shader.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cfloat>

#ifndef BONEINFO_DEFINED
#define BONEINFO_DEFINED
struct BoneInfo {
    int id;
    glm::mat4 offset; 
};
#endif

class Model {
private:
    std::string mapMixamoTextureName(const std::string& originalPath, const std::string& textureType);
protected:
    std::string m_directory;
    std::vector<Texture> m_texturesLoaded;
    Assimp::Importer m_importer;
    std::vector<std::unique_ptr<Mesh>> m_meshes;
    const aiScene* m_scene = nullptr;

    std::map<std::string, BoneInfo> m_BoneInfoMap;
    int m_BoneCounter = 0;

    // Metodi di caricamento Assimp
    void loadModel(const std::string& path);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName);
    GLuint textureFromFile(const char* path, const std::string& directory, bool gamma = false);

    virtual std::unique_ptr<Mesh> processMesh(aiMesh* mesh, const aiScene* scene);
    virtual void processNode(aiNode* node, const aiScene* scene);

public:
    // Costruttore e distruttore
    Model(const std::string& path, bool gamma = false);
    virtual ~Model();

    // Metodi di rendering
    void render(Shader& shader);
    void cleanup();

    void getModelMinMax(glm::vec3& outMin, glm::vec3& outMax) const;

    std::map<std::string, BoneInfo>& GetBoneInfoMap() { return m_BoneInfoMap; }
    const std::map<std::string, BoneInfo>& GetBoneInfoMap() const { return m_BoneInfoMap; }
    int& GetBoneCount() { return m_BoneCounter; }
    int GetBoneCount() const { return m_BoneCounter; }

    size_t getMeshCount() const;
    Mesh* getMesh(size_t index) const;

    glm::vec3 calculateModelCenter() const;
    float calculateModelBoundingRadius() const;
    glm::vec3 calculateModelBoundingBox() const;

    std::vector<glm::vec3> getFragmentCenters() const;
    std::vector<float> getFragmentRadii() const;
};