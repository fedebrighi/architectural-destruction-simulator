
#include "Character.h"
#include "Shader.h"
#include "AnimatedModel.h"
#include "Animation.h"
#include "DebugBB.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/constants.hpp>
#include <cfloat> 

const Model* Character::GetModelPtr() const { return model.get(); }

Character::Character(const std::string& modelPath)
    : position(0.0f), rotation(0.0f), velocity(0.0f),
    currentState(CharacterState::IDLE), previousState(CharacterState::IDLE),
    m_punchCooldown(0.0f), m_isVisible(true), m_renderScale(2.0f) {

    std::cout << "=== CHARACTER CONSTRUCTOR START ===" << std::endl;
    
    model = std::make_unique<AnimatedModel>("mostro.fbx");

    AnimatedModel* animModel = dynamic_cast<AnimatedModel*>(model.get());
    if (animModel) {
        std::cout << "AnimatedModel created successfully" << std::endl;
        std::cout << "Model bone count: " << animModel->GetBoneCount() << std::endl;
    }
    else {
        std::cout << "ERROR: Failed to cast to AnimatedModel!" << std::endl;
    }

    std::cout << "Loading animations..." << std::endl;
    idleAnimation = std::make_unique<Animation>("Idle_mostro.fbx", model.get());
    walkAnimation = std::make_unique<Animation>("Walking_mostro.fbx", model.get());
    punchAnimation = std::make_unique<Animation>("Punch_mostro.fbx", model.get());

    idleAnimation->SetLooping(true);
    walkAnimation->SetLooping(true);
    punchAnimation->SetLooping(false);

    currentAnimation = idleAnimation.get();

    std::cout << "=== CHARACTER CONSTRUCTOR END ===" << std::endl;
}

Character::~Character() {}

void Character::Render(Shader& shader) {
    if (!model || !m_isVisible) return;

    shader.use();
    glm::mat4 modelMatrix = GetModelMatrix();
    shader.setMat4("model", modelMatrix);

    shader.setBool("hasTexture", true);

    shader.setVec3("lightPos", glm::vec3(10.0f, 10.0f, 10.0f));
    shader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    shader.setVec3("viewPos", glm::vec3(0.0f, 5.0f, 10.0f));

    shader.setVec3("materialDiffuse", glm::vec3(0.8f, 0.7f, 0.6f));
    shader.setVec3("materialSpecular", glm::vec3(0.2f, 0.2f, 0.2f));
    shader.setFloat("materialShininess", 32.0f);

    // Gestione animazioni
    AnimatedModel* animModel = dynamic_cast<AnimatedModel*>(model.get());
    if (currentAnimation && animModel) {
        std::vector<glm::mat4> boneTransforms = currentAnimation->GetFinalBoneMatrices();

        static int debugCounter = 0;
        if (debugCounter % 180 == 0) {
            std::cout << "=== CHARACTER RENDER DEBUG ===" << std::endl;
            std::cout << "State: " << static_cast<int>(currentState) << std::endl;
            std::cout << "Visible: " << m_isVisible << std::endl;
            std::cout << "Scale: " << m_renderScale << std::endl;
            std::cout << "Animation: " << (currentAnimation == idleAnimation.get() ? "IDLE" :
                currentAnimation == walkAnimation.get() ? "WALK" : "PUNCH") << std::endl;
            std::cout << "Position: " << glm::to_string(position) << std::endl;
            std::cout << "=============================" << std::endl;
        }
        debugCounter++;

        int maxBones = std::min(static_cast<int>(boneTransforms.size()), 100);

        if (maxBones > 0) {
            for (int i = 0; i < maxBones; ++i) {
                std::string uniformName = "boneMatrices[" + std::to_string(i) + "]";
                shader.setMat4(uniformName, boneTransforms[i]);
            }
            shader.setBool("hasAnimation", true);
            shader.setInt("boneCount", maxBones);
        }
        else {
            shader.setBool("hasAnimation", false);
            shader.setInt("boneCount", 0);
        }
    }
    else {
        shader.setBool("hasAnimation", false);
        shader.setInt("boneCount", 0);
    }

    model->render(shader);
}

void Character::Move(const glm::vec3& direction, float speed, float deltaTime) {
    // Controlli di sicurezza
    if (std::isnan(direction.x) || std::isnan(direction.y) || std::isnan(direction.z)) return;
    if (std::isinf(direction.x) || std::isinf(direction.y) || std::isinf(direction.z)) return;

    // Non muovere se sta punchando
    if (currentState == CharacterState::PUNCHING) return;

    float moveLength = glm::length(direction);

    if (moveLength > 0.1f) {
        glm::vec3 normalizedDir = glm::normalize(direction);
        float moveDistance = speed * deltaTime;

        position += normalizedDir * moveDistance;

        // Gestione stato
        if (currentState != CharacterState::WALKING) {
            StartWalking();
        }

        // Gestione rotazione
        float targetRotation = atan2(normalizedDir.x, normalizedDir.z);
        if (!std::isnan(targetRotation) && !std::isinf(targetRotation)) {
            float rotDiff = targetRotation - rotation;

            while (rotDiff > glm::pi<float>()) rotDiff -= 2 * glm::pi<float>();
            while (rotDiff < -glm::pi<float>()) rotDiff += 2 * glm::pi<float>();

            float rotSpeed = 10.0f;
            if (abs(rotDiff) > 0.1f) {
                rotation += rotDiff * rotSpeed * deltaTime;
            }
            else {
                rotation = targetRotation;
            }
        }
    }
    else {
        if (currentState == CharacterState::WALKING) {
            StopWalking();
        }
    }
}

void Character::Update(float deltaTime) {
    // Aggiorna cooldown del punch
    if (m_punchCooldown > 0.0f) {
        m_punchCooldown -= deltaTime;
    }

    // Aggiorna stato del personaggio
    UpdateState();

    // Aggiorna animazione
    if (currentAnimation) {
        currentAnimation->Update(deltaTime);
    }

    if (currentState == CharacterState::FINISHED_PUNCH) {
        static float finishTimer = 0.0f;
        finishTimer += deltaTime;

        if (finishTimer > 0.5f) { // Aspetta mezzo secondo
            ResetToIdle();
            finishTimer = 0.0f;
        }
    }

    // Debug ridotto
    static int debugCounter = 0;
    if (debugCounter % 300 == 0) { // Ogni 5 secondi
        std::cout << "Character Update - State: " << static_cast<int>(currentState)
            << " | Visible: " << m_isVisible
            << " | Cooldown: " << m_punchCooldown << std::endl;
    }
    debugCounter++;
}

bool Character::IsNearBuilding(const glm::vec3& buildingPos, float threshold) {
    return glm::distance(position, buildingPos) < threshold;
}

bool Character::IsNearTree(const glm::vec3& treePos, float threshold) {
    return glm::distance(position, treePos) < threshold;
}

bool Character::IsNearObject(const glm::vec3& objectPos, float threshold) {
    return glm::distance(position, objectPos) < threshold;
}

void Character::InteractWithBuilding() {
    if (m_punchCooldown <= 0.0f && currentState != CharacterState::PUNCHING) {
        std::cout << "Character punching object!" << std::endl;
        StartPunching();
        m_punchCooldown = 3.0f;
    }
}

void Character::InteractWithTree() {
    if (m_punchCooldown <= 0.0f && currentState != CharacterState::PUNCHING) {
        std::cout << "Character punching tree!" << std::endl;
        StartPunching();
        m_punchCooldown = 2.0f; 
    }
}

void Character::StartWalking() {
    if (currentState == CharacterState::IDLE || currentState == CharacterState::FINISHED_PUNCH) {
        currentState = CharacterState::WALKING;
        SwitchAnimation(walkAnimation.get());
    }
}

void Character::StopWalking() {
    if (currentState == CharacterState::WALKING) {
        currentState = CharacterState::IDLE;
        SwitchAnimation(idleAnimation.get());
    }
}

void Character::StartPunching() {
    if (currentState != CharacterState::PUNCHING) {
        previousState = currentState;
        currentState = CharacterState::PUNCHING;
        SwitchAnimation(punchAnimation.get());
        punchAnimation->Reset();
    }
}

bool Character::HasFinishedPunching() const {
    return currentState == CharacterState::PUNCHING && punchAnimation->IsFinished();
}

void Character::UpdateState() {
    if (currentState == CharacterState::PUNCHING && punchAnimation->IsFinished()) {
        currentState = CharacterState::FINISHED_PUNCH;
        std::cout << "Character finished punching!" << std::endl;
    }
}

void Character::ResetToIdle() {
    std::cout << "Character resetting to idle state" << std::endl;
    currentState = CharacterState::IDLE;
    SwitchAnimation(idleAnimation.get());
    m_punchCooldown = 0.0f;

    m_isVisible = true;
    m_renderScale = 2.0f;
}

void Character::SwitchAnimation(Animation* newAnimation) {
    if (currentAnimation != newAnimation) {
        currentAnimation = newAnimation;
        currentAnimation->Reset();
    }
}

void Character::SetVisible(bool visible) {
    m_isVisible = visible;
    if (visible) {
        std::cout << "Character set to visible" << std::endl;
    }
    else {
        std::cout << "Character set to invisible" << std::endl;
    }
}

void Character::SetRenderScale(float scale) {
    m_renderScale = std::max(0.01f, scale); 
}

glm::mat4 Character::GetModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, rotation, glm::vec3(0.0f, 1.0f, 0.0f));

    const float importScale = 0.01f;
    float finalScale = importScale * m_renderScale; 
    model = glm::scale(model, glm::vec3(finalScale));

    return model;
}

// GETTER
glm::vec3 Character::GetPosition() const { return position; }
CharacterState Character::GetState() const { return currentState; }
bool Character::IsVisible() const { return m_isVisible; }
float Character::GetPunchCooldown() const { return m_punchCooldown; }


// SETTER
void Character::SetPosition(const glm::vec3& pos) {
    position = pos;
}

AABB Character::ComputeDynamicAABBWorld() const {
    // Model matrix del personaggio (world space)
    glm::mat4 Mw = GetModelMatrix();

    auto* anim = currentAnimation;
    auto* animModel = dynamic_cast<AnimatedModel*>(model.get());

    if (!anim || !animModel) {
        glm::vec3 minB(FLT_MAX), maxB(-FLT_MAX);
        for (size_t i = 0; i < model->getMeshCount(); ++i) {
            auto* mesh = model->getMesh(i);
            if (!mesh) continue;
            const auto& verts = mesh->getVertices();
            for (const auto& v : verts) {
                glm::vec3 p = glm::vec3(Mw * glm::vec4(v.Position, 1.0f));
                minB = glm::min(minB, p);
                maxB = glm::max(maxB, p);
            }
        }
        return { (minB + maxB) * 0.5f, (maxB - minB) * 0.5f };
    }

    std::vector<glm::mat4> finals = anim->GetFinalBoneMatrices();
    const auto& boneMap = animModel->GetBoneInfoMap();  // nome -> {id, offset}
    glm::mat4 Ginv = anim->GetGlobalInverseTransform();
    glm::mat4 G = glm::inverse(Ginv);

    glm::vec3 minB(FLT_MAX), maxB(-FLT_MAX);

    auto addBonePoint = [&](const std::string& name, float r) {
        auto it = boneMap.find(name);
        if (it == boneMap.end()) return;
        int id = it->second.id;
        if (id < 0 || id >= (int)finals.size()) return;

        // Global ≈ G * final * inverse(offset)
        glm::mat4 Global = G * finals[id] * glm::inverse(it->second.offset);
        glm::vec3 pW = glm::vec3(Mw * Global * glm::vec4(0, 0, 0, 1));

        minB = glm::min(minB, pW - glm::vec3(r));
        maxB = glm::max(maxB, pW + glm::vec3(r));
        };

    // helper per la punta proiettata
    auto includePoint = [&](const glm::vec3& p, float r) {
        minB = glm::min(minB, p - glm::vec3(r));
        maxB = glm::max(maxB, p + glm::vec3(r));
        };
    auto boneWorldM = [&](const std::string& name, glm::mat4& outW)->bool {
        auto it = boneMap.find(name);
        if (it == boneMap.end()) return false;
        int id = it->second.id;
        if (id < 0 || id >= (int)finals.size()) return false;
        glm::mat4 Global = G * finals[id] * glm::inverse(it->second.offset);
        outW = Mw * Global;   // world-space
        return true;
        };

    // 3) Ossa principali (Mixamo)
    const float rCore = 0.25f * m_renderScale;
    const float rLimb = 0.30f * m_renderScale;

    addBonePoint("mixamorig:Hips", rCore);
    addBonePoint("mixamorig:Spine", rCore);
    addBonePoint("mixamorig:Spine1", rCore);
    addBonePoint("mixamorig:Spine2", rCore);
    addBonePoint("mixamorig:Neck", rCore);
    addBonePoint("mixamorig:Head", rCore);

    addBonePoint("mixamorig:LeftUpLeg", rLimb);
    addBonePoint("mixamorig:LeftLeg", rLimb);
    addBonePoint("mixamorig:LeftFoot", rLimb);
    addBonePoint("mixamorig:RightUpLeg", rLimb);
    addBonePoint("mixamorig:RightLeg", rLimb);
    addBonePoint("mixamorig:RightFoot", rLimb);

    addBonePoint("mixamorig:LeftArm", rLimb);
    addBonePoint("mixamorig:LeftForeArm", rLimb);
    addBonePoint("mixamorig:LeftHand", rLimb);
    addBonePoint("mixamorig:RightArm", rLimb);
    addBonePoint("mixamorig:RightForeArm", rLimb);
    addBonePoint("mixamorig:RightHand", rLimb);

    {
        glm::mat4 Hw, Fw;
        if (boneWorldM("mixamorig:RightHand", Hw) && boneWorldM("mixamorig:RightForeArm", Fw)) {
            glm::vec3 hand = glm::vec3(Hw * glm::vec4(0, 0, 0, 1));
            glm::vec3 fore = glm::vec3(Fw * glm::vec4(0, 0, 0, 1));
            glm::vec3 vFH = hand - fore;
            float seg = glm::length(vFH);
            if (seg > 1e-5f) {
                glm::vec3 dirFH = vFH / seg; 

                // assi locali della mano in world (colonne della matrice)
                glm::vec3 ax = glm::normalize(glm::vec3(Hw[0])); // X
                glm::vec3 az = glm::normalize(glm::vec3(Hw[2])); // Z 

                glm::vec3 cand[4] = { ax, -ax, az, -az };
                int best = 0; float bestDot = -1e9f;
                for (int i = 0; i < 4; ++i) {
                    float d = glm::dot(dirFH, cand[i]);
                    if (d > bestDot) { bestDot = d; best = i; }
                }
                glm::vec3 handAxisDir = cand[best];

                // estensioni (regolabili)
                float L1 = seg * 1.20f + 0.20f * m_renderScale; // oltre la mano lungo avambraccio
                float L2 = seg * 0.80f + 0.20f * m_renderScale; // lungo l'asse "forward" della mano

                glm::vec3 tip1 = hand + dirFH * L1; // punta
                glm::vec3 tip2 = hand + handAxisDir * L2; // punta lungo asse mano

                includePoint(tip1, 0.10f * m_renderScale);
                includePoint(tip2, 0.10f * m_renderScale);
            }
        }
    }

    {
        glm::mat4 Hw, Fw, Tw;
        if (boneWorldM("mixamorig:LeftHand", Hw) &&
            boneWorldM("mixamorig:LeftForeArm", Fw) &&
            boneWorldM("mixamorig:Spine2", Tw))             // riferimento torace
        {
            // punti e direzioni
            glm::vec3 hand = glm::vec3(Hw * glm::vec4(0, 0, 0, 1));
            glm::vec3 fore = glm::vec3(Fw * glm::vec4(0, 0, 0, 1));
            glm::vec3 torso = glm::vec3(Tw * glm::vec4(0, 0, 0, 1));

            glm::vec3 vFH = hand - fore;
            float seg = glm::length(vFH);
            glm::vec3 dirFH = (seg > 1e-5f) ? vFH / seg : glm::normalize(glm::vec3(Hw[2]));

            // assi locali mano (world)
            glm::vec3 ax = glm::normalize(glm::vec3(Hw[0]));   // X mano
            glm::vec3 az = glm::normalize(glm::vec3(Hw[2]));   // Z mano 

            glm::vec3 outward = glm::normalize(hand - torso);
            glm::vec3 cand[4] = { ax, -ax, az, -az };
            int best = 0; float bestDot = -1e9f;
            for (int i = 0; i < 4; i++) { float d = glm::dot(outward, cand[i]); if (d > bestDot) { bestDot = d; best = i; } }
            glm::vec3 handAxisDir = cand[best];


            float L_alongForearm = seg * 1.50f + 0.35f * m_renderScale;
            float L_forwardHand = std::max(seg * 1.10f, 0.80f * m_renderScale);

            glm::vec3 tip1 = hand + dirFH * L_alongForearm;
            glm::vec3 tip2 = hand + handAxisDir * L_forwardHand;  // asse più esterno della mano

            includePoint(tip1, 0.14f * m_renderScale);
            includePoint(tip2, 0.14f * m_renderScale);

            float sidePad = 0.20f * m_renderScale;
            includePoint(hand + ax * sidePad, 0.06f * m_renderScale);
            includePoint(hand - ax * sidePad, 0.06f * m_renderScale);
        }
    }

    if (minB.x == FLT_MAX) {
        glm::vec3 minS(FLT_MAX), maxS(-FLT_MAX);
        for (size_t i = 0; i < model->getMeshCount(); ++i) {
            auto* mesh = model->getMesh(i);
            if (!mesh) continue;
            const auto& verts = mesh->getVertices();
            for (const auto& v : verts) {
                glm::vec3 p = glm::vec3(Mw * glm::vec4(v.Position, 1.0f));
                minS = glm::min(minS, p);
                maxS = glm::max(maxS, p);
            }
        }
        return { (minS + maxS) * 0.5f, (maxS - minS) * 0.5f };
    }

    // AABB in world-space
    return { (minB + maxB) * 0.5f, (maxB - minB) * 0.5f };
}

float Character::GetPunchNormalizedTime() const {
    return (currentAnimation) ? currentAnimation->GetNormalizedTime() : 0.0f;
}

Capsule Character::GetRightFistCapsuleWorld() const {
    Capsule cap{ glm::vec3(0), glm::vec3(0), 0.18f * m_renderScale }; // raggio da rifinire
    auto* anim = currentAnimation;
    auto* animModel = dynamic_cast<AnimatedModel*>(model.get());
    if (!anim || !animModel) return cap;

    auto finals = anim->GetFinalBoneMatrices();
    const auto& map = animModel->GetBoneInfoMap();
    auto find = [&](const char* n)->const auto* {
        auto it = map.find(n); return (it == map.end() ? nullptr : &it->second);
        };
    auto* h = find("mixamorig:RightHand"), * f = find("mixamorig:RightForeArm");
    if (!h || !f) return cap;

    glm::mat4 G = glm::inverse(anim->GetGlobalInverseTransform());
    glm::mat4 Mw = GetModelMatrix();

    glm::mat4 Hw = Mw * (G * finals[h->id] * glm::inverse(h->offset));
    glm::mat4 Fw = Mw * (G * finals[f->id] * glm::inverse(f->offset));

    cap.a = glm::vec3(Fw * glm::vec4(0, 0, 0, 1));
    cap.b = glm::vec3(Hw * glm::vec4(0, 0, 0, 1));
    return cap;
}
