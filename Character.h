#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include "DebugBB.h"

class Model;
class Animation;
class Shader;

enum class CharacterState {
    IDLE,
    WALKING,
    PUNCHING,
    FINISHED_PUNCH
};

class Character {
public:
    Character(const std::string& modelPath);
    ~Character();

    const Model* GetModelPtr() const;

    void Update(float deltaTime);
    void Render(Shader& shader);
    void Move(const glm::vec3& direction, float speed, float deltaTime);

    bool IsNearBuilding(const glm::vec3& buildingPos, float threshold);
    bool IsNearTree(const glm::vec3& treePos, float threshold);
    bool IsNearObject(const glm::vec3& objectPos, float threshold); // Generico

    void InteractWithBuilding();
    void InteractWithTree();

    void StartWalking();
    void StopWalking();
    void StartPunching();
    bool HasFinishedPunching() const;

    // State management
    void UpdateState();
    void ResetToIdle(); // Reset dopo esplosione
    void SwitchAnimation(Animation* newAnimation);

    // Visibilità
    void SetVisible(bool visible);
    void SetRenderScale(float scale);
    bool IsVisible() const;

    glm::vec3 GetPosition() const;
    CharacterState GetState() const;
    float GetPunchCooldown() const;
    glm::mat4 GetModelMatrix() const;

    void SetPosition(const glm::vec3& pos);

    AABB ComputeDynamicAABBWorld() const;

    Capsule GetRightFistCapsuleWorld() const;
    float   GetPunchNormalizedTime() const;

private:
    std::unique_ptr<Model> model;
    std::unique_ptr<Animation> idleAnimation;
    std::unique_ptr<Animation> walkAnimation;
    std::unique_ptr<Animation> punchAnimation;
    Animation* currentAnimation;

    glm::vec3 position;
    float rotation;
    glm::vec3 velocity;

    CharacterState currentState;
    CharacterState previousState;

    float m_punchCooldown;      // Previene spam di punch
    bool m_isVisible;           // Controlla visibilità
    float m_renderScale;
};