#pragma once

#include <glm/glm.hpp>
#include <vector>

class PhysicsSystemCPU {
private:
    struct CPU_Fragment {
        glm::vec3 position;
        glm::vec3 initialPosition;
        glm::vec3 velocity;
        float radius;
        float mass;
        float structuralIntegrity;  // 1.0 = intatto, 0.0 = esploso
    };

    std::vector<CPU_Fragment> m_fragments;

    // Parametri esplosione
    glm::vec3 m_explosionCenter;
    float m_explosionForce;
    float m_explosionRadius;
    float m_currentTime = 0.0f;

    // Parametri fisica (stessi valori del GPU)
    glm::vec3 m_gravity = glm::vec3(0.0f, -9.81f, 0.0f);
    float m_airDamping = 0.15f;
    float m_restitution = 0.3f;

    bool m_active = false;
    float m_lastUpdateTimeMs = 0.0f;
    float m_groundLevel = -3.0f;

public:
    void initialize(const std::vector<glm::vec3>& centers, const std::vector<float>& radii);
    void startExplosion(const glm::vec3& center, float force, float radius);
    void update(float deltaTime);
    void reset();

    void setGroundLevel(float level) { m_groundLevel = level; }
    float getGroundLevel() const { return m_groundLevel; }

    std::vector<glm::mat4> getFragmentTransforms() const;

    int getFragmentCount() const { return m_fragments.size(); }
    float getLastUpdateTime() const { return m_lastUpdateTimeMs; }
    bool isActive() const { return m_active; }
};