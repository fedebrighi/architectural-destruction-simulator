#include "PhysicsSystem.h"
#include "ComputeShader.h"
#include "StateManager.h"
#include <iostream>
#include <algorithm>
#include <numeric>

PhysicsSystem::PhysicsSystem() : m_stateManager(nullptr) {
    loadRealisticPreset();
}

PhysicsSystem::~PhysicsSystem() {
}


bool PhysicsSystem::initialize(StateManager* stateManager) {

    m_stateManager = stateManager; // Può essere nullptr per tree physics

    // Create compute shader
    m_computeShader = std::make_unique<ComputeShader>();
    if (!m_computeShader->initialize()) {
        std::cerr << "PhysicsSystem: Failed to initialize compute shader" << std::endl;
        return false;
    }

    applyForceParameters();

    if (stateManager) {
        std::cout << "PhysicsSystem initialized successfully (with StateManager)" << std::endl;
    }
    else {
        std::cout << "PhysicsSystem initialized successfully (standalone mode)" << std::endl;
    }

    return true;
}

void PhysicsSystem::setupFragments(const std::vector<glm::vec3>& centers, const std::vector<float>& radii) {
    if (!m_computeShader) {
        std::cerr << "PhysicsSystem: Compute shader not initialized" << std::endl;
        return;
    }

    if (centers.empty() || radii.empty() || centers.size() != radii.size()) {
        std::cerr << "PhysicsSystem: Invalid fragment data" << std::endl;
        return;
    }

    m_computeShader->initializeFragments(centers, radii);
    applyForceParameters();

    std::cout << "PhysicsSystem: " << centers.size() << " fragments initialized" << std::endl;
}

void PhysicsSystem::startExplosion(const glm::vec3& center) {
    if (!m_computeShader) {
        std::cerr << "PhysicsSystem: Cannot start explosion - compute shader not initialized" << std::endl;
        return;
    }

    std::cout << "Starting explosion at (" << center.x << ", " << center.y << ", " << center.z << ")" << std::endl;

    m_computeShader->startExplosion(center, m_forceParams.explosionForce, m_forceParams.explosionRadius);

    auto& params = const_cast<PhysicsParams&>(m_computeShader->getPhysicsParams());
    params.maxTime = m_forceParams.explosionDuration;
}

void PhysicsSystem::resetSimulation() {
    if (m_computeShader) {
        m_computeShader->resetSimulation();
        std::cout << "PhysicsSystem: Simulation reset" << std::endl;
    }
}

void PhysicsSystem::pauseSimulation() {
    if (m_computeShader) {
        m_computeShader->pauseSimulation();
    }
}

void PhysicsSystem::resumeSimulation() {
    if (m_computeShader) {
        m_computeShader->resumeSimulation();
    }
}

void PhysicsSystem::update(float deltaTime) {
    if (!m_computeShader) return;

    if (deltaTime <= 0.0f || deltaTime > 0.1f) {
        deltaTime = 1.0f / 60.0f;
    }

    float scaledDeltaTime = deltaTime * m_forceParams.timeScale;
    m_computeShader->update(scaledDeltaTime);
    m_computeShader->dispatch();

    if (m_computeShader->isSimulationActive()) {
        m_computeShader->downloadFragmentDataFromGPU();
    }

    updateDebugInfo();

    if (m_debugUpdateCallback) {
        m_debugUpdateCallback(m_debugInfo);
    }
}


void PhysicsSystem::applyForceParameters() {
    if (!m_computeShader) return;

    glm::vec3 scaledGravity = m_forceParams.gravity * m_forceParams.gravityScale;
    m_computeShader->setGravity(scaledGravity);
    m_computeShader->setAirDamping(m_forceParams.airDamping);
    m_computeShader->setExplosionParameters(glm::vec3(0.0f), m_forceParams.explosionForce, m_forceParams.explosionRadius);
    m_computeShader->setGroundLevel(-0.60f);
}

void PhysicsSystem::updateDebugInfo() {
    if (!m_computeShader) return;

    m_debugInfo.simulationActive = m_computeShader->isSimulationActive();
    m_debugInfo.explosionProgress = m_computeShader->getSimulationProgress();
    m_debugInfo.activeFragments = m_computeShader->getNumFragments();

    if ((m_readbackCounter++ % m_readbackPeriod) != 0) {
        return;
    }

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    const auto fragmentData = m_computeShader->getFragmentData();

    if (fragmentData.empty()) {
        m_debugInfo.averageVelocity = 0.0f;
        m_debugInfo.maxVelocity = 0.0f;
        m_debugInfo.totalKineticEnergy = 0.0f;
        return;
    }

    float sumSpeed = 0.0f;
    float maxSpeed = 0.0f;
    float totalKineticEnergy = 0.0f;

    for (const auto& fragment : fragmentData) {
        const float speed = glm::length(fragment.velocity);
        sumSpeed += speed;
        if (speed > maxSpeed) maxSpeed = speed;
        totalKineticEnergy += 0.5f * fragment.mass * speed * speed;
    }

    m_debugInfo.averageVelocity = sumSpeed / static_cast<float>(fragmentData.size());
    m_debugInfo.maxVelocity = maxSpeed;
    m_debugInfo.totalKineticEnergy = totalKineticEnergy;
}



void PhysicsSystem::setForceParameters(const ForceParameters& params) {
    m_forceParams = params;
    applyForceParameters();
}

void PhysicsSystem::setGravityScale(float scale) {
    m_forceParams.gravityScale = scale;
    if (m_computeShader) {
        glm::vec3 scaledGravity = m_forceParams.gravity * scale;
        m_computeShader->setGravity(scaledGravity);
    }
}

void PhysicsSystem::setExplosionForce(float force) {
    m_forceParams.explosionForce = force;
}

void PhysicsSystem::setExplosionRadius(float radius) {
    m_forceParams.explosionRadius = radius;
}

void PhysicsSystem::setExplosionDuration(float duration) {
    m_forceParams.explosionDuration = duration;
}

void PhysicsSystem::setAirDamping(float damping) {
    m_forceParams.airDamping = damping;
    if (m_computeShader) {
        m_computeShader->setAirDamping(damping);
    }
}

void PhysicsSystem::setTimeScale(float scale) {
    m_forceParams.timeScale = std::max(0.1f, std::min(3.0f, scale));
}

void PhysicsSystem::setRestitution(float restitution) {
    m_forceParams.restitution = std::max(0.0f, std::min(1.0f, restitution));
}

// PRESETS

void PhysicsSystem::loadRealisticPreset() {
    std::cout << "Loading REALISTIC physics preset..." << std::endl;

    m_forceParams.gravity = glm::vec3(0.0f, -9.81f, 0.0f);
    m_forceParams.gravityScale = 1.0f;           // Un po' meno gravità del'albero
    m_forceParams.explosionForce = 10.0f;        // Un po' più forza (palazzo più pesante)
    m_forceParams.explosionRadius = 7.0f;        // Raggio leggermente maggiore
    m_forceParams.explosionDuration = 4.0f;
    m_forceParams.airDamping = 0.35f;            // Leggermente meno attrito
    m_forceParams.timeScale = 1.0f;
    m_forceParams.restitution = 0.25f;           // Un po' più rimbalzo

    applyForceParameters();
}

void PhysicsSystem::loadDisintegrationPreset() {
    std::cout << "Loading DISINTEGRATION physics preset..." << std::endl;

    m_forceParams.gravity = glm::vec3(0.0f, -9.81f, 0.0f);
    m_forceParams.gravityScale = 3.2f;      
    m_forceParams.explosionForce = 8.0f;    
    m_forceParams.airDamping = 0.4f;       
    m_forceParams.explosionRadius = 10.0f;       
    m_forceParams.explosionDuration = 3.0f;
    m_forceParams.timeScale = 0.6f;             
    m_forceParams.restitution = 0.4f;           

    applyForceParameters();
}

std::vector<glm::mat4> PhysicsSystem::getFragmentTransforms() {
    if (m_computeShader) {
        return m_computeShader->getFragmentTransforms();
    }
    return {};
}

bool PhysicsSystem::isInitialized() const {
    return m_computeShader && m_computeShader->isInitialized();
}

bool PhysicsSystem::isSimulationActive() const {
    return m_computeShader && m_computeShader->isSimulationActive();
}

int PhysicsSystem::getFragmentCount() const {
    return m_computeShader ? m_computeShader->getNumFragments() : 0;
}

void PhysicsSystem::setDebugUpdateCallback(std::function<void(const PhysicsDebugInfo&)> callback) {
    m_debugUpdateCallback = callback;
}

void PhysicsSystem::setGroundLevel(float level) {
    if (m_computeShader) {
        m_computeShader->setGroundLevel(level);
        std::cout << "PhysicsSystem: Ground level set to " << level << std::endl;
    }
}

void PhysicsSystem::printForceParameters() const {
    std::cout << "=== PHYSICS PARAMETERS ===" << std::endl;
    std::cout << "Gravity Scale: " << m_forceParams.gravityScale << std::endl;
    std::cout << "Explosion Force: " << m_forceParams.explosionForce << std::endl;
    std::cout << "Explosion Radius: " << m_forceParams.explosionRadius << std::endl;
    std::cout << "Explosion Duration: " << m_forceParams.explosionDuration << std::endl;
    std::cout << "Time Scale: " << m_forceParams.timeScale << std::endl;
    std::cout << "=========================" << std::endl;
}

void PhysicsSystem::printDebugInfo() const {
    std::cout << "=== PHYSICS DEBUG INFO ===" << std::endl;
    std::cout << "Active Fragments: " << m_debugInfo.activeFragments << std::endl;
    std::cout << "Simulation Active: " << (m_debugInfo.simulationActive ? "YES" : "NO") << std::endl;
    std::cout << "Explosion Progress: " << (m_debugInfo.explosionProgress * 100.0f) << "%" << std::endl;
    std::cout << "Average Velocity: " << m_debugInfo.averageVelocity << " m/s" << std::endl;
    std::cout << "Max Velocity: " << m_debugInfo.maxVelocity << " m/s" << std::endl;
    std::cout << "==========================" << std::endl;
}