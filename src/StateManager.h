#pragma once

#include <functional>
#include <chrono>
#include <glm/glm.hpp>

// STATI DELL'APPLICAZIONE
enum class AppState {
    INIT,
    INTACT_BUILDING,
    EXPLOSION_TRANSITION,
    FRAGMENTS_SIMULATION,
    PAUSED
};

// PARAMETRI DELL'ESPLOSIONE
struct ExplosionData {
    glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
    float force = 10.0f;
    float radius = 5.0f;
    float duration = 2.0f;
    float currentTime = 0.0f;
    bool active = false;

    void reset() {
        currentTime = 0.0f;
        active = false;
    }

    bool isFinished() const {
        return currentTime >= duration;
    }

    float getProgress() const {
        return active ? (currentTime / duration) : 0.0f;
    }
};

class StateManager {
private:
    AppState m_currentState;
    AppState m_previousState;

    ExplosionData m_explosion;

    std::chrono::high_resolution_clock::time_point m_stateStartTime;
    float m_stateElapsedTime;

    std::function<void(AppState, AppState)> m_stateChangeCallback;
    std::function<void()> m_explosionStartCallback;
    std::function<void()> m_explosionEndCallback;

    void notifyStateChange(AppState newState);

public:
    StateManager();
    ~StateManager();

    void initialize();
    void update(float deltaTime);
    void setState(AppState newState);

    void triggerExplosion(const glm::vec3& center = glm::vec3(0.0f, 0.0f, 0.0f),
        float force = 100.0f, float radius = 5.0f);
    void resetExplosion();
    void pauseSimulation();
    void resumeSimulation();

    AppState getCurrentState() const { return m_currentState; }
    AppState getPreviousState() const { return m_previousState; }
    const ExplosionData& getExplosionData() const { return m_explosion; }
    float getStateElapsedTime() const { return m_stateElapsedTime; }

    bool isIntact() const { return m_currentState == AppState::INTACT_BUILDING; }
    bool isExploding() const { return m_currentState == AppState::EXPLOSION_TRANSITION; }
    bool isFragmented() const { return m_currentState == AppState::FRAGMENTS_SIMULATION; }
    bool isPaused() const { return m_currentState == AppState::PAUSED; }
    bool isExplosionActive() const { return m_explosion.active; }

    // Callbacks
    void setStateChangeCallback(std::function<void(AppState, AppState)> callback) {
        m_stateChangeCallback = callback;
    }
    void setExplosionStartCallback(std::function<void()> callback) {
        m_explosionStartCallback = callback;
    }
    void setExplosionEndCallback(std::function<void()> callback) {
        m_explosionEndCallback = callback;
    }

    // Utilities
    static std::string stateToString(AppState state);
};