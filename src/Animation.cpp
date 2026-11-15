#define GLM_ENABLE_EXPERIMENTAL
#include "Animation.h"
#include "Model.h"
#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include <algorithm>

namespace AssimpGLMHelpers {
    glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from) {
        glm::mat4 to;
        to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
        to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
        to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
        to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
        return to;
    }

    glm::vec3 GetGLMVec(const aiVector3D& vec) {
        return glm::vec3(vec.x, vec.y, vec.z);
    }

    glm::quat GetGLMQuat(const aiQuaternion& quat) {
        return glm::quat(quat.w, quat.x, quat.y, quat.z);
    }
}

AnimationChannel::AnimationChannel(const std::string& boneName, int boneID, const aiNodeAnim* channel)
    : m_BoneName(boneName), m_BoneID(boneID)
{
    for (unsigned int i = 0; i < channel->mNumPositionKeys; ++i)
    {
        m_Positions.push_back({ AssimpGLMHelpers::GetGLMVec(channel->mPositionKeys[i].mValue), (float)channel->mPositionKeys[i].mTime });
    }

    for (unsigned int i = 0; i < channel->mNumRotationKeys; ++i)
    {
        m_Rotations.push_back({ AssimpGLMHelpers::GetGLMQuat(channel->mRotationKeys[i].mValue), (float)channel->mRotationKeys[i].mTime });
    }

    for (unsigned int i = 0; i < channel->mNumScalingKeys; ++i)
    {
        m_Scales.push_back({ AssimpGLMHelpers::GetGLMVec(channel->mScalingKeys[i].mValue), (float)channel->mScalingKeys[i].mTime });
    }
}

glm::mat4 AnimationChannel::GetLocalTransform(float animationTime)
{
    glm::vec3 position = InterpolatePosition(animationTime);
    glm::quat rotation = InterpolateRotation(animationTime);
    glm::vec3 scale = InterpolateScaling(animationTime);

    glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 rotation_matrix = glm::mat4_cast(rotation);
    glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), scale);

    return translation_matrix * rotation_matrix * scale_matrix;
}

int AnimationChannel::GetPositionIndex(float animationTime)
{
    for (size_t i = 0; i < m_Positions.size() - 1; ++i)
    {
        if (animationTime < m_Positions[i + 1].timeStamp)
            return static_cast<int>(i);
    }
    return static_cast<int>(m_Positions.size() - 1);
}

int AnimationChannel::GetRotationIndex(float animationTime)
{
    for (size_t i = 0; i < m_Rotations.size() - 1; ++i)
    {
        if (animationTime < m_Rotations[i + 1].timeStamp)
            return static_cast<int>(i);
    }
    return static_cast<int>(m_Rotations.size() - 1);
}

int AnimationChannel::GetScaleIndex(float animationTime)
{
    for (size_t i = 0; i < m_Scales.size() - 1; ++i)
    {
        if (animationTime < m_Scales[i + 1].timeStamp)
            return static_cast<int>(i);
    }
    return static_cast<int>(m_Scales.size() - 1);
}

glm::vec3 AnimationChannel::InterpolatePosition(float animationTime)
{
    if (m_Positions.size() == 1) return m_Positions[0].position;

    int p0Index = GetPositionIndex(animationTime);
    int p1Index = p0Index + 1;
    float scaleFactor = (animationTime - m_Positions[p0Index].timeStamp) / (m_Positions[p1Index].timeStamp - m_Positions[p0Index].timeStamp);
    return glm::mix(m_Positions[p0Index].position, m_Positions[p1Index].position, scaleFactor);
}

glm::quat AnimationChannel::InterpolateRotation(float animationTime)
{
    if (m_Rotations.size() == 1) return m_Rotations[0].orientation;

    int r0Index = GetRotationIndex(animationTime);
    int r1Index = r0Index + 1;
    float scaleFactor = (animationTime - m_Rotations[r0Index].timeStamp) / (m_Rotations[r1Index].timeStamp - m_Rotations[r0Index].timeStamp);
    return glm::slerp(m_Rotations[r0Index].orientation, m_Rotations[r1Index].orientation, scaleFactor);
}

glm::vec3 AnimationChannel::InterpolateScaling(float animationTime)
{
    if (m_Scales.size() == 1) return m_Scales[0].scale;

    int s0Index = GetScaleIndex(animationTime);
    int s1Index = s0Index + 1;
    float scaleFactor = (animationTime - m_Scales[s0Index].timeStamp) / (m_Scales[s1Index].timeStamp - m_Scales[s0Index].timeStamp);
    return glm::mix(m_Scales[s0Index].scale, m_Scales[s1Index].scale, scaleFactor);
}

Animation::Animation(const std::string& animationPath, Model* model)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }
    m_GlobalInverseTransform = glm::inverse(AssimpGLMHelpers::ConvertMatrixToGLMFormat(scene->mRootNode->mTransformation));
    std::cout << "ANIMATION: Using GlobalInverseTransform from root node" << std::endl;

    auto animation = scene->mAnimations[0];
    m_Duration = (float)animation->mDuration;
    m_TicksPerSecond = (float)animation->mTicksPerSecond;
    if (m_TicksPerSecond == 0.0f) m_TicksPerSecond = 25.0f;

    ReadHierarchyData(m_RootNode, scene->mRootNode);
    ReadAnimationChannels(animation, model);

    m_FinalBoneMatrices.reserve(100);
    for (int i = 0; i < 100; i++) {
        m_FinalBoneMatrices.push_back(glm::mat4(1.0f));
    }

    std::cout << "Animation loaded: Duration=" << m_Duration << ", TicksPerSecond=" << m_TicksPerSecond << std::endl;
}


Animation::~Animation() {}

void Animation::ReadHierarchyData(AssimpNodeData& dest, const aiNode* src)
{
    assert(src);

    dest.name = src->mName.C_Str();
    dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
    dest.childrenCount = src->mNumChildren;

    for (int i = 0; i < src->mNumChildren; i++)
    {
        AssimpNodeData newData;
        ReadHierarchyData(newData, src->mChildren[i]);
        dest.children.push_back(newData);
    }
}

void Animation::ReadAnimationChannels(const aiAnimation* animation, Model* model)
{
    if (!model) {
        std::cerr << "ERRORE: Modello non valido passato all'animazione." << std::endl;
        return;
    }

    m_BoneInfoMap = model->GetBoneInfoMap();
    int& boneCount = model->GetBoneCount();

    std::cout << "Animation loading - Model has " << boneCount << " bones" << std::endl;
    std::cout << "Animation has " << animation->mNumChannels << " channels" << std::endl;

    int missingBonesCount = 0;

    for (unsigned int i = 0; i < animation->mNumChannels; ++i)
    {
        auto channel = animation->mChannels[i];
        std::string boneName = channel->mNodeName.C_Str();

        if (m_BoneInfoMap.find(boneName) != m_BoneInfoMap.end()) {
            m_AnimationChannels.emplace_back(boneName, m_BoneInfoMap[boneName].id, channel);
        }
        else {
            missingBonesCount++;
            std::cout << "  WARNING: Animation bone '" << boneName << "' not found in model" << std::endl;
        }
    }

    if (missingBonesCount > 0) {
        std::cout << "WARNING: " << missingBonesCount << " animation bones not found in model" << std::endl;
        std::cout << "This may cause animation artifacts if those bones are important" << std::endl;
    }

    std::cout << "Final animation channels created: " << m_AnimationChannels.size() << std::endl;
}
AnimationChannel* Animation::FindAnimationChannel(const std::string& boneName)
{
    auto iter = std::find_if(m_AnimationChannels.begin(), m_AnimationChannels.end(),
        [&](const AnimationChannel& channel) {
            return channel.GetBoneName() == boneName;
        });
    if (iter == m_AnimationChannels.end()) return nullptr;
    else return &(*iter);
}

void Animation::Update(float deltaTime) {
    if (!IsPlaying()) return;

    m_CurrentTime += deltaTime * m_TicksPerSecond;

    if (m_CurrentTime >= m_Duration) {
        if (m_IsLooping) {
            m_CurrentTime = fmod(m_CurrentTime, m_Duration);
            static int loopCount = 0;
            loopCount++;
            if (loopCount % 10 == 0) { 
                std::cout << "Animation looped (" << loopCount << " times)" << std::endl;
            }
        }
        else {
            m_CurrentTime = m_Duration;
            m_IsPlaying = false;
        }
    }

    static int debugCounter = 0;
    if (debugCounter % 300 == 0) {
        std::cout << "Animation time: " << m_CurrentTime << "/" << m_Duration
            << " | Looping: " << (m_IsLooping ? "YES" : "NO") << std::endl;
    }
    debugCounter++;
}

void Animation::CalculateBoneTransformRecursive(const AssimpNodeData* node, glm::mat4 parentTransform)
{
    std::string nodeName = node->name;
    glm::mat4 nodeTransform = node->transformation;

    AnimationChannel* channel = FindAnimationChannel(nodeName);

    if (channel) {
        nodeTransform = channel->GetLocalTransform(m_CurrentTime);
    }

    glm::mat4 globalTransformation = parentTransform * nodeTransform;

    if (m_BoneInfoMap.count(nodeName)) {
        int boneID = m_BoneInfoMap.at(nodeName).id;

        if (boneID >= 0 && boneID < static_cast<int>(m_FinalBoneMatrices.size())) {
            glm::mat4 offsetMatrix = m_BoneInfoMap.at(nodeName).offset;

            m_FinalBoneMatrices[boneID] = m_GlobalInverseTransform * globalTransformation * offsetMatrix;
        }
    }

    for (int i = 0; i < node->childrenCount; i++) {
        CalculateBoneTransformRecursive(&node->children[i], globalTransformation);
    }
}

void Animation::DebugBoneTransforms() {
    std::cout << "=== ANIMATION BONE TRANSFORMS DEBUG ===" << std::endl;
    std::cout << "Current time: " << m_CurrentTime << std::endl;
    std::cout << "Duration: " << m_Duration << std::endl;
    std::cout << "Animation channels: " << m_AnimationChannels.size() << std::endl;

    for (int i = 0; i < 10 && i < m_FinalBoneMatrices.size(); i++) {
        const glm::mat4& mat = m_FinalBoneMatrices[i];
        bool isIdentity = (mat[0][0] == 1.0f && mat[1][1] == 1.0f &&
            mat[2][2] == 1.0f && mat[3][3] == 1.0f &&
            mat[3][0] == 0.0f && mat[3][1] == 0.0f && mat[3][2] == 0.0f);

        std::cout << "Bone " << i << ": " << (isIdentity ? "IDENTITY" : "ANIMATED") << std::endl;
        if (!isIdentity) {
            std::cout << "  Translation: (" << mat[3][0] << ", " << mat[3][1] << ", " << mat[3][2] << ")" << std::endl;
        }
    }
    std::cout << "=========================================" << std::endl;
}

std::vector<glm::mat4> Animation::GetFinalBoneMatrices()
{
    CalculateBoneTransformRecursive(&m_RootNode, glm::mat4(1.0f));
    return m_FinalBoneMatrices;
}