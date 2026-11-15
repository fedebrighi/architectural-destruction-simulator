#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>
#include <functional>
#include <vector>

class ComputeShader;
class StateManager;

struct ForceParameters {
    glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);
    float gravityScale = 1.0f;

    float explosionForce = 5.0f;   
    float explosionRadius = 8.0f;
    float explosionDuration = 6.0f;
 
    // Resistenza dell'Aria
    float airDamping = 0.05f;

    float restitution = 0.3f;       
    float friction = 0.7f;       

    float timeScale = 0.5f;   
};

struct PhysicsDebugInfo {
    int activeFragments = 0;
    float averageVelocity = 0.0f;
    float maxVelocity = 0.0f;
    float totalKineticEnergy = 0.0f;
    float explosionProgress = 0.0f;
    bool simulationActive = false;
};

class PhysicsSystem {
private:
    std::unique_ptr<ComputeShader> m_computeShader;
    StateManager* m_stateManager;

    ForceParameters m_forceParams;
    PhysicsDebugInfo m_debugInfo;
    int m_readbackCounter = 0;
    int m_readbackPeriod = 30;

    std::function<void(const PhysicsDebugInfo&)> m_debugUpdateCallback;

    void updateDebugInfo();
    void applyForceParameters();

public:
    PhysicsSystem();
    ~PhysicsSystem();

    bool initialize(StateManager* stateManager);
    void setupFragments(const std::vector<glm::vec3>& centers, const std::vector<float>& radii);

    void startExplosion(const glm::vec3& center);
    void resetSimulation();
    void pauseSimulation();
    void resumeSimulation();
    void update(float deltaTime);

    void setForceParameters(const ForceParameters& params);
    const ForceParameters& getForceParameters() const { return m_forceParams; }

    void setGravityScale(float scale);
    void setExplosionForce(float force);
    void setExplosionRadius(float radius);
    void setExplosionDuration(float duration);
    void setAirDamping(float damping);
    void setTimeScale(float scale);
    void setRestitution(float restitution);
    void setGroundLevel(float level);

    void loadRealisticPreset();
    void loadDisintegrationPreset();

    std::vector<glm::mat4> getFragmentTransforms();
    const PhysicsDebugInfo& getDebugInfo() const { return m_debugInfo; }

    bool isInitialized() const;
    bool isSimulationActive() const;
    int getFragmentCount() const;

    void setDebugUpdateCallback(std::function<void(const PhysicsDebugInfo&)> callback);
    void printForceParameters() const;
    void printDebugInfo() const;
};