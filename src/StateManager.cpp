#include "StateManager.h"
#include <iostream>
#include <glm/glm.hpp>

StateManager::StateManager()
    : m_currentState(AppState::INIT), m_previousState(AppState::INIT), m_stateElapsedTime(0.0f) {
    m_stateStartTime = std::chrono::high_resolution_clock::now();
}

StateManager::~StateManager() {
}

void StateManager::initialize() {
    std::cout << "StateManager initialized" << std::endl;
    setState(AppState::INTACT_BUILDING);
}

void StateManager::update(float deltaTime) {
    auto currentTime = std::chrono::high_resolution_clock::now();
    m_stateElapsedTime = std::chrono::duration<float>(currentTime - m_stateStartTime).count();

    if (m_explosion.active) {
        m_explosion.currentTime += deltaTime;

        if (m_currentState == AppState::EXPLOSION_TRANSITION && m_explosion.isFinished()) {
            setState(AppState::FRAGMENTS_SIMULATION);
            if (m_explosionEndCallback) {
                m_explosionEndCallback();
            }
        }
    }

    switch (m_currentState) {
    case AppState::INIT:
        break;

    case AppState::INTACT_BUILDING:
        break;

    case AppState::EXPLOSION_TRANSITION:
        break;

    case AppState::FRAGMENTS_SIMULATION:
        break;

    case AppState::PAUSED:
        return;
    }
}

void StateManager::setState(AppState newState) {
    if (newState == m_currentState) return;

    AppState oldState = m_currentState;
    m_previousState = m_currentState;
    m_currentState = newState;

    m_stateStartTime = std::chrono::high_resolution_clock::now();
    m_stateElapsedTime = 0.0f;

    std::cout << "State changed: " << stateToString(oldState)
        << " -> " << stateToString(newState) << std::endl;

    notifyStateChange(newState);
}

void StateManager::triggerExplosion(const glm::vec3& center, float force, float radius) {
    if (m_currentState == AppState::INTACT_BUILDING) {
        std::cout << "EXPLOSION TRIGGERED!" << std::endl;
        std::cout << "  Center: (" << center.x << ", " << center.y << ", " << center.z << ")" << std::endl;
        std::cout << "  Force: " << force << std::endl;
        std::cout << "  Radius: " << radius << std::endl;
  
        m_explosion.center = center;
        m_explosion.force = force;
        m_explosion.radius = radius;
        m_explosion.currentTime = 0.0f;
        m_explosion.active = true;

        setState(AppState::EXPLOSION_TRANSITION);

        if (m_explosionStartCallback) {
            m_explosionStartCallback();
        }
    }
    else {
        std::cout << "Cannot trigger explosion in current state: "
            << stateToString(m_currentState) << std::endl;
    }
}

void StateManager::resetExplosion() {
    std::cout << "Resetting explosion - returning to intact building" << std::endl;

    m_explosion.reset();
    setState(AppState::INTACT_BUILDING);
}

void StateManager::pauseSimulation() {
    if (m_currentState != AppState::PAUSED) {
        std::cout << "Simulation paused" << std::endl;
        setState(AppState::PAUSED);
    }
}

void StateManager::resumeSimulation() {
    if (m_currentState == AppState::PAUSED) {
        std::cout << "Simulation resumed" << std::endl;
        setState(m_previousState);
    }
}

void StateManager::notifyStateChange(AppState newState) {
    if (m_stateChangeCallback) {
        m_stateChangeCallback(m_previousState, newState);
    }
}

std::string StateManager::stateToString(AppState state) {
    switch (state) {
    case AppState::INIT: return "INIT";
    case AppState::INTACT_BUILDING: return "INTACT_BUILDING";
    case AppState::EXPLOSION_TRANSITION: return "EXPLOSION_TRANSITION";
    case AppState::FRAGMENTS_SIMULATION: return "FRAGMENTS_SIMULATION";
    case AppState::PAUSED: return "PAUSED";
    default: return "UNKNOWN";
    }
}