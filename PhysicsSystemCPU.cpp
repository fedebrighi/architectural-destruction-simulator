#include "PhysicsSystemCPU.h"
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <iostream>
#include <random>

// Funzione per generare il noise
float noise(glm::vec3 p) {
    return glm::fract(glm::sin(glm::dot(p, glm::vec3(12.9898f, 78.233f, 45.164f))) * 43758.5453f);
}

void PhysicsSystemCPU::initialize(const std::vector<glm::vec3>& centers, const std::vector<float>& radii) {
    m_fragments.clear();
    for (size_t i = 0; i < centers.size(); ++i) {
        CPU_Fragment f;
        f.position = centers[i];
        f.initialPosition = centers[i];
        f.velocity = glm::vec3(0.0f);
        f.radius = radii[i];
        f.mass = 1.0f;
        f.structuralIntegrity = 1.0f;
        m_fragments.push_back(f);
    }
    m_active = false;
    m_currentTime = 0.0f;
}

void PhysicsSystemCPU::startExplosion(const glm::vec3& center, float force, float radius) {
    m_currentTime = 0.0f;
    m_explosionCenter = center;
    m_explosionForce = force;
    m_explosionRadius = radius;

    for (size_t i = 0; i < m_fragments.size(); ++i) {
        auto& f = m_fragments[i];

        glm::vec3 toFragment = f.initialPosition - center;
        float distance = glm::length(toFragment);

        if (distance < radius) {
            glm::vec3 explosionDirection = distance > 0.001f ? glm::normalize(toFragment) : glm::vec3(0.0f, 1.0f, 0.0f);

            // Falloff più graduale per migliore dispersione
            float forceFalloff = 1.0f - (distance / radius);
            forceFalloff = glm::pow(forceFalloff, 1.5f);  

            // + casualità per dispersione maggiore
            float randomFactor = noise(f.initialPosition * 10.0f + float(i));
            glm::vec3 randomDirection = glm::vec3(
                noise(f.initialPosition + glm::vec3(1.0f, 0.0f, 0.0f)) - 0.5f,
                noise(f.initialPosition + glm::vec3(0.0f, 1.0f, 0.0f)) * 0.3f,  // Meno random su Y
                noise(f.initialPosition + glm::vec3(0.0f, 0.0f, 1.0f)) - 0.5f
            ) * 0.6f;  

            // Direzione finale con più componente orizzontale
            glm::vec3 finalDirection = glm::normalize(explosionDirection + randomDirection);

            // Velocità con enfasi sulla dispersione orizzontale
            glm::vec3 explosionVelocity = finalDirection * force * forceFalloff;

            // Componente verticale bilanciata
            explosionVelocity.y = explosionVelocity.y * 0.7f + force * 0.2f;

            // BOOST ORIZZONTALE per dispersione
            explosionVelocity.x *= 1.3f;  // ← Aumenta dispersione X
            explosionVelocity.z *= 1.3f;  // ← Aumenta dispersione Z

            f.velocity = explosionVelocity;
            f.structuralIntegrity = 0.0f;
        }
    }

    m_active = true;
}

void PhysicsSystemCPU::update(float deltaTime) {
    if (!m_active) {
        m_lastUpdateTimeMs = 0.0f;
        return;
    }

    auto start = std::chrono::high_resolution_clock::now();
    m_currentTime += deltaTime;

    for (auto& f : m_fragments) {
        if (f.structuralIntegrity <= 0.0f) {
            // Gravità normale
            f.velocity += m_gravity * deltaTime;

            // Air damping
            float speed = glm::length(f.velocity);
            if (speed > 0.001f) {
                f.velocity *= (1.0f - m_airDamping * 0.5f * deltaTime);
            }

            // Aggiorna posizione
            f.position += f.velocity * deltaTime;

            float fragmentGroundLevel = m_groundLevel + f.radius + 0.05f;

            if (f.position.y <= fragmentGroundLevel) {
                f.position.y = fragmentGroundLevel;

                if (f.velocity.y < -0.1f) {
                    f.velocity.y = -f.velocity.y * m_restitution;
                }
                else {
                    f.velocity.y = 0.0f;
                }

                f.velocity.x *= 0.9f;
                f.velocity.z *= 0.9f;

                if (glm::length(f.velocity) < 0.01f) {
                    f.velocity = glm::vec3(0.0f);
                }
            }

            f.position = glm::clamp(f.position, glm::vec3(-100.0f), glm::vec3(100.0f));

            if (f.position.y < m_groundLevel - 1.0f) {
                f.position.y = fragmentGroundLevel;
                f.velocity = glm::vec3(0.0f);
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    m_lastUpdateTimeMs = std::chrono::duration<float, std::milli>(end - start).count();
}

std::vector<glm::mat4> PhysicsSystemCPU::getFragmentTransforms() const {
    std::vector<glm::mat4> transforms;
    for (const auto& f : m_fragments) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), f.position);
        transforms.push_back(model); 
    }
    return transforms;
}

void PhysicsSystemCPU::reset() {
    for (auto& f : m_fragments) {
        f.position = f.initialPosition;
        f.velocity = glm::vec3(0.0f);
        f.structuralIntegrity = 1.0f;
    }
    m_active = false;
    m_currentTime = 0.0f;
}