#include "Model.h"
#include <iostream>
#include <algorithm> 
#include <cmath>   
#include "stb_image.h"

Model::Model(const std::string& path, bool gamma) {
    std::cout << "=== MODEL CONSTRUCTOR START ===" << std::endl;
    std::cout << "Loading model: " << path << std::endl;

    // Inizializza i membri
    m_BoneCounter = 0;
    m_BoneInfoMap.clear();

    loadModel(path);

    std::cout << "Model loaded. Bone counter: " << m_BoneCounter << std::endl;
    std::cout << "Bone info map size: " << m_BoneInfoMap.size() << std::endl;
    std::cout << "=== MODEL CONSTRUCTOR END ===" << std::endl;
}

Model::~Model() {
    cleanup();
}

void Model::loadModel(const std::string& path) {
    // Read file via ASSIMP
    m_scene = m_importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    // Check for errors
    if (!m_scene || m_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !m_scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP:: " << m_importer.GetErrorString() << std::endl;
        return;
    }

    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) {
        m_directory = "."; 
    }
    else {
        m_directory = path.substr(0, pos);
    }

    std::cout << "Model directory: " << m_directory << std::endl;
    std::cout << "Scene has " << m_scene->mNumMeshes << " meshes" << std::endl;

    processNode(m_scene->mRootNode, m_scene);

    std::cout << "Model processing complete. Total meshes: " << m_meshes.size() << std::endl;
}

void Model::processNode(aiNode* node, const aiScene* scene) {

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        m_meshes.push_back(processMesh(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

std::unique_ptr<Mesh> Model::processMesh(aiMesh* mesh, const aiScene* scene) {

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Texture> textures;

    // Vertici
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        glm::vec3 vector;

        // Posizioni
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;

        // Normali
        if (mesh->HasNormals()) {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        }

        if (mesh->mTextureCoords[0]) {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;

            if (mesh->HasTangentsAndBitangents()) {
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;

                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            }
        }
        else {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        // Bones init
        vertex.BoneIDs = glm::ivec4(-1);
        vertex.Weights = glm::vec4(0.0f);

        vertices.push_back(vertex);
    }

    // Indici
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    // Materiali
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    auto diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());


    if (diffuseMaps.empty()) {
        aiColor3D kd(1.0f, 1.0f, 1.0f);
        if (material->Get(AI_MATKEY_COLOR_DIFFUSE, kd) == AI_SUCCESS) {
            GLuint solidId; glGenTextures(1, &solidId);
            glBindTexture(GL_TEXTURE_2D, solidId);
            unsigned char rgba[4] = {
                (unsigned char)std::round(kd.r * 255.0f),
                (unsigned char)std::round(kd.g * 255.0f),
                (unsigned char)std::round(kd.b * 255.0f),
                255
            };
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            Texture t; t.id = solidId; t.type = "texture_diffuse"; t.path = "__kd_color";
            textures.push_back(t);
        }
    }

    auto specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

    auto normalMapsA = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal");
    auto normalMapsB = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    normalMapsA.insert(normalMapsA.end(), normalMapsB.begin(), normalMapsB.end());
    textures.insert(textures.end(), normalMapsA.begin(), normalMapsA.end());

    return std::make_unique<Mesh>(vertices, indices, textures);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName) {
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str; mat->GetTexture(type, i, &str);
        std::string originalPath = str.C_Str();

        std::string mappedFileName = mapMixamoTextureName(originalPath, typeName);

        // Evita duplicati
        bool skip = false;
        for (unsigned int j = 0; j < m_texturesLoaded.size(); j++) {
            if (m_texturesLoaded[j].path == mappedFileName) {
                textures.push_back(m_texturesLoaded[j]);
                skip = true; break;
            }
        }
        if (!skip) {
            Texture texture;
            texture.id = textureFromFile(mappedFileName.c_str(), m_directory); 
            texture.type = typeName;
            texture.path = mappedFileName;
            textures.push_back(texture);
            m_texturesLoaded.push_back(texture);
        }
    }
    return textures;
}

std::string Model::mapMixamoTextureName(const std::string& originalPath, const std::string& textureType) {
    std::string lowerPath = originalPath;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);

    // Gestione specifica per il modello Mutant
    if (lowerPath.find("mutant") != std::string::npos) {
        if (textureType == "texture_diffuse") return "Mutant_diffuse.png";
        if (textureType == "texture_normal")  return "Mutant_normal.png";
    }

    std::string fileName = originalPath;
    size_t lastSlash = fileName.find_last_of("/\\");
    if (lastSlash != std::string::npos) fileName = fileName.substr(lastSlash + 1);
    return fileName;
}

GLuint Model::textureFromFile(const char* path, const std::string& directory, bool gamma) {
    std::string filename = path ? std::string(path) : std::string();

    for (auto& c : filename) if (c == '\\') c = '/';

    std::string fullPath;
    if (filename.find('/') == std::string::npos) fullPath = directory + "/" + filename; else fullPath = filename;

    std::cout << "Attempting to load texture: " << fullPath << std::endl;

    GLuint textureID; glGenTextures(1, &textureID);

    int width = 0, height = 0, nrComponents = 0;
    unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &nrComponents, 0);

    if (!data) {
        std::string alt0 = directory + "/" + filename;
        if (alt0 != fullPath) {
            std::cout << "Retrying with: " << alt0 << std::endl;
            data = stbi_load(alt0.c_str(), &width, &height, &nrComponents, 0);
        }

        // Estrai solo il nome file
        std::string filenameOnly = filename;
        size_t pos = filenameOnly.find_last_of('/');
        if (pos != std::string::npos) filenameOnly = filenameOnly.substr(pos + 1);

        if (!data) {
            std::string alt1 = directory + "/" + filenameOnly;
            std::cout << "Retrying with: " << alt1 << std::endl;
            data = stbi_load(alt1.c_str(), &width, &height, &nrComponents, 0);
        }
        if (!data) {
            std::string alt2 = std::string("textures/") + filenameOnly;
            std::cout << "Retrying with: " << alt2 << std::endl;
            data = stbi_load(alt2.c_str(), &width, &height, &nrComponents, 0);
        }
        if (!data) {
            std::string alt3 = std::string("textures/") + filename;
            std::cout << "Retrying with: " << alt3 << std::endl;
            data = stbi_load(alt3.c_str(), &width, &height, &nrComponents, 0);
        }
    }

    if (data) {
        GLenum format = GL_RGB;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        std::cout << "SUCCESS: Loaded texture: " << filename << " (" << width << "x" << height << ", " << nrComponents << " components)" << std::endl;
    }
    else {
        std::cout << "ERROR: Failed to load texture after all attempts: " << filename << std::endl;
        std::cout << "STB Error: " << stbi_failure_reason() << std::endl;

        glBindTexture(GL_TEXTURE_2D, textureID);
        unsigned char whitePixel[] = { 255,255,255,255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        std::cout << "Using fallback white texture" << std::endl;
    }

    return textureID;
}

void Model::render(Shader& shader) {
    for (auto& mesh : m_meshes) {
        mesh->render(shader);
    }
}

void Model::cleanup() {
    m_meshes.clear();
    m_texturesLoaded.clear();
    m_BoneInfoMap.clear();
    m_BoneCounter = 0;
}

glm::vec3 Model::calculateModelCenter() const {
    glm::vec3 center(0.0f);
    int totalVertices = 0;

    for (const auto& mesh : m_meshes) {
        glm::vec3 meshCenter = mesh->calculateCenter();
        const auto& vertices = mesh->getVertices();
        center += meshCenter * static_cast<float>(vertices.size());
        totalVertices += static_cast<int>(vertices.size());
    }

    if (totalVertices > 0) center /= static_cast<float>(totalVertices);
    return center;
}

float Model::calculateModelBoundingRadius() const {
    float maxRadius = 0.0f;
    glm::vec3 center = calculateModelCenter();

    for (const auto& mesh : m_meshes) {
        const auto& vertices = mesh->getVertices();
        for (const auto& vertex : vertices) {
            float distance = glm::length(vertex.Position - center);
            maxRadius = std::max(maxRadius, distance);
        }
    }
    return maxRadius;
}

glm::vec3 Model::calculateModelBoundingBox() const {
    glm::vec3 minBounds(FLT_MAX);
    glm::vec3 maxBounds(-FLT_MAX);

    for (const auto& mesh : m_meshes) {
        const auto& vertices = mesh->getVertices();
        for (const auto& vertex : vertices) {
            minBounds = glm::min(minBounds, vertex.Position);
            maxBounds = glm::max(maxBounds, vertex.Position);
        }
    }

    return maxBounds - minBounds;
}

std::vector<glm::vec3> Model::getFragmentCenters() const {
    std::vector<glm::vec3> centers; centers.reserve(m_meshes.size());
    for (const auto& mesh : m_meshes) centers.push_back(mesh->calculateCenter());
    return centers;
}

std::vector<float> Model::getFragmentRadii() const {
    std::vector<float> radii; radii.reserve(m_meshes.size());
    for (const auto& mesh : m_meshes) radii.push_back(mesh->calculateBoundingRadius());
    return radii;
}

void Model::getModelMinMax(glm::vec3& minB, glm::vec3& maxB) const {
    minB = glm::vec3(FLT_MAX);
    maxB = glm::vec3(-FLT_MAX);

    for (const auto& mesh : m_meshes) {
        const auto& vertices = mesh->getVertices();
        for (const auto& v : vertices) {
            minB = glm::min(minB, v.Position);
            maxB = glm::max(maxB, v.Position);
        }
    }
}

size_t Model::getMeshCount() const { return m_meshes.size(); }

Mesh* Model::getMesh(size_t index) const { return index < m_meshes.size() ? m_meshes[index].get() : nullptr; }
