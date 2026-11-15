#include "AnimatedModel.h"
#include <iostream>

class AssimpGLMHelpers {
public:
    static glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from) {
        glm::mat4 to;
        to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
        to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
        to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
        to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
        return to;
    }
};

AnimatedModel::AnimatedModel(const std::string& path) : Model(path) {
    std::cout << "=== ANIMATEDMODEL CONSTRUCTOR START ===" << std::endl;
    std::cout << "Loading animated model from: " << path << std::endl;

    std::cout << "After Model constructor - boneCounter: " << m_BoneCounter << std::endl;
    std::cout << "After Model constructor - boneInfoMap size: " << m_BoneInfoMap.size() << std::endl;

    std::cout << "POST-PROCESSING: Creating bones manually from known Mixamo structure..." << std::endl;
    m_meshes.clear();
    processNode(m_scene->mRootNode, m_scene);
    std::cout << "After post-processing: boneCounter = " << m_BoneCounter << std::endl;
    std::cout << "After post-processing: boneInfoMap size = " << m_BoneInfoMap.size() << std::endl;

    DebugBoneMapping();

    std::cout << "=== ANIMATEDMODEL CONSTRUCTOR END ===" << std::endl;
}

void AnimatedModel::DebugBoneMapping() {
    std::cout << "=== BONE MAPPING DEBUG ===" << std::endl;
    std::cout << "Total bones in model: " << m_BoneCounter << std::endl;
    std::cout << "=========================" << std::endl;
}

void AnimatedModel::CreateMixamoBones() {
    std::cout << "Creating standard Mixamo bone structure..." << std::endl;
    std::vector<std::string> mixamoBones = {
        "mixamorig:Hips", "mixamorig:Head", "mixamorig:LeftArm", "mixamorig:LeftFoot",
        "mixamorig:LeftForeArm", "mixamorig:RightArm", "mixamorig:RightFoot", "mixamorig:RightForeArm",
        "mixamorig:Spine", "mixamorig:Spine1", "mixamorig:Spine2", "mixamorig:Neck",
        "mixamorig:LeftHand", "mixamorig:RightHand", "mixamorig:LeftUpLeg", "mixamorig:RightUpLeg",
        "mixamorig:LeftLeg", "mixamorig:RightLeg"
    };

    for (const std::string& boneName : mixamoBones) {
        if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end()) {
            BoneInfo newBoneInfo;
            newBoneInfo.id = m_BoneCounter;
            newBoneInfo.offset = glm::mat4(1.0f);
            m_BoneInfoMap[boneName] = newBoneInfo;
            m_BoneCounter++;
            std::cout << "Created bone: " << boneName << " (ID: " << newBoneInfo.id << ")" << std::endl;
        }
    }
    std::cout << "Created " << m_BoneCounter << " bones manually" << std::endl;
}

void AnimatedModel::SetVertexBoneDataToDefault(Vertex& vertex) {
    for (int i = 0; i < 4; i++) {
        vertex.BoneIDs[i] = -1;
        vertex.Weights[i] = 0.0f;
    }
}

void AnimatedModel::SetVertexBoneData(Vertex& vertex, int boneID, float weight) {
    for (int i = 0; i < 4; ++i) {
        if (vertex.BoneIDs[i] < 0) {
            vertex.Weights[i] = weight;
            vertex.BoneIDs[i] = boneID;
            break;
        }
    }
}

void AnimatedModel::ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene) {
    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
        int boneID = -1;
        std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

        if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end()) {
            BoneInfo newBoneInfo;
            newBoneInfo.id = m_BoneCounter;

            newBoneInfo.offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);

            m_BoneInfoMap[boneName] = newBoneInfo;
            boneID = m_BoneCounter;
            m_BoneCounter++;
        }
        else {
            boneID = m_BoneInfoMap[boneName].id;
            m_BoneInfoMap[boneName].offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
        }

        auto weights = mesh->mBones[boneIndex]->mWeights;
        int numWeights = mesh->mBones[boneIndex]->mNumWeights;

        for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex) {
            int vertexId = weights[weightIndex].mVertexId;
            float weight = weights[weightIndex].mWeight;

            if (vertexId >= 0 && vertexId < static_cast<int>(vertices.size())) {
                if (weight > 0.0f && weight <= 1.0f) {
                    SetVertexBoneData(vertices[vertexId], boneID, weight);
                }
                else {
                    std::cout << "    WARNING: Invalid weight " << weight << " for vertex " << vertexId << std::endl;
                }
            }
            else {
                std::cout << "    ERROR: Vertex ID " << vertexId << " out of bounds (max: " << vertices.size() << ")" << std::endl;
            }
        }
    }

    int verticesWithoutWeights = 0;
    for (size_t i = 0; i < vertices.size(); i++) {
        float totalWeight = vertices[i].Weights.x + vertices[i].Weights.y + vertices[i].Weights.z + vertices[i].Weights.w;
        if (totalWeight < 0.01f) {
            verticesWithoutWeights++;
        }
    }

    if (verticesWithoutWeights > 0) {
        std::cout << "INFO: " << verticesWithoutWeights << " vertices have no bone weights (will use bind pose)" << std::endl;
    }

    for (auto& v : vertices) {
        float total = v.Weights.x + v.Weights.y + v.Weights.z + v.Weights.w;
        if (total > 0.0f) {
            v.Weights /= total;
        }
    }
}

std::unique_ptr<Mesh> AnimatedModel::processMesh(aiMesh* mesh, const aiScene* scene) {

    auto processedMesh = Model::processMesh(mesh, scene);
    auto& vertices = processedMesh->vertices;

    for (auto& vertex : vertices) {
        SetVertexBoneDataToDefault(vertex);
    }
    ExtractBoneWeightForVertices(vertices, mesh, scene);
    processedMesh->updateVertexData();

    return processedMesh;
}

void AnimatedModel::processNode(aiNode* node, const aiScene* scene) {

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        m_meshes.push_back(processMesh(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

void AnimatedModel::FixBoneWeightsPostConstruction() {
    std::cout << "FixBoneWeightsPostConstruction skipped (already handled during mesh processing)" << std::endl;
}