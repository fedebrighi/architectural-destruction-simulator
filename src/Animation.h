#pragma once

#include <vector>
#include <map>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

class Model;

#ifndef BONEINFO_DEFINED
#define BONEINFO_DEFINED
struct BoneInfo
{
    int id;
    glm::mat4 offset;
};
#endif

struct KeyPosition
{
    glm::vec3 position;
    float timeStamp;
};

struct KeyRotation
{
    glm::quat orientation;
    float timeStamp;
};

struct KeyScale
{
    glm::vec3 scale;
    float timeStamp;
};

class AnimationChannel
{
public:
    AnimationChannel(const std::string& boneName, int boneID, const aiNodeAnim* channel);

    glm::mat4 GetLocalTransform(float animationTime);
    std::string GetBoneName() const { return m_BoneName; }
    int GetBoneID() const { return m_BoneID; }

private:
    glm::vec3 InterpolatePosition(float animationTime);
    glm::quat InterpolateRotation(float animationTime);
    glm::vec3 InterpolateScaling(float animationTime);

    int GetPositionIndex(float animationTime);
    int GetRotationIndex(float animationTime);
    int GetScaleIndex(float animationTime);

    std::vector<KeyPosition> m_Positions;
    std::vector<KeyRotation> m_Rotations;
    std::vector<KeyScale> m_Scales;
    std::string m_BoneName;
    int m_BoneID;
};

struct AssimpNodeData
{
    glm::mat4 transformation;
    std::string name;
    int childrenCount;
    std::vector<AssimpNodeData> children;
};

class Animation
{
public:
    Animation(const std::string& animationPath, Model* model);
    ~Animation();

    void Update(float dt);
    void DebugBoneTransforms();
    glm::mat4 m_GlobalInverseTransform;
    std::vector<glm::mat4> GetFinalBoneMatrices();
    std::vector<glm::mat4> GetBoneTransforms() { return GetFinalBoneMatrices(); }

    void Reset() { m_CurrentTime = 0.0f; m_IsPlaying = true; }
    bool IsPlaying() const { return m_IsPlaying; }
    bool IsFinished() const { return m_CurrentTime >= m_Duration && !m_IsLooping; }
    float GetCurrentTime() const { return m_CurrentTime; }

    void Play() { m_IsPlaying = true; }
    void Pause() { m_IsPlaying = false; }
    void SetLooping(bool loop) { m_IsLooping = loop; }

    float GetTicksPerSecond() const { return m_TicksPerSecond; }
    float GetDuration() const { return m_Duration; }
    const AssimpNodeData& GetRootNode() const { return m_RootNode; }
    const std::map<std::string, BoneInfo>& GetBoneInfoMap() const { return m_BoneInfoMap; }

    AnimationChannel* FindAnimationChannel(const std::string& boneName);

    bool IsLooping() const { return m_IsLooping; }

    const glm::mat4& GetGlobalInverseTransform() const { return m_GlobalInverseTransform; }

    float GetNormalizedTime() const {
        return (m_Duration > 0.f) ? fmod(m_CurrentTime, m_Duration) / m_Duration : 0.f;
    }

private:
    void ReadHierarchyData(AssimpNodeData& dest, const aiNode* src);
    void ReadAnimationChannels(const aiAnimation* animation, Model* model);
    void CalculateBoneTransformRecursive(const AssimpNodeData* node, glm::mat4 parentTransform);

    float m_Duration;
    float m_TicksPerSecond;
    std::vector<AnimationChannel> m_AnimationChannels;
    AssimpNodeData m_RootNode;
    std::map<std::string, BoneInfo> m_BoneInfoMap;
    std::vector<glm::mat4> m_FinalBoneMatrices;

    float m_CurrentTime = 0.0f;
    bool m_IsPlaying = true;
    bool m_IsLooping = true;
};