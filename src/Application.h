#pragma once

#include "Window.h"
#include "PerformanceMonitor.h"
#include "Logger.h"
#include "DebugUI.h"
#include "Character.h"
#include "AnimatedModel.h"
#include "PhysicsSystemCPU.h"
#include "DebugBB.h"
#include <memory>
#include <chrono>

class Camera;
class Renderer;
class Skybox;
class Model;
class StateManager;
class InputHandler;
class PhysicsSystem;

class Application {
private:
    std::unique_ptr<Window> m_window;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<Skybox> m_skybox;
    std::unique_ptr<Model> m_buildingModel;
    std::unique_ptr<Model> m_fragmentsModel;
    std::unique_ptr<StateManager> m_stateManager;
    std::unique_ptr<InputHandler> m_inputHandler;
    std::unique_ptr<PhysicsSystem> m_physicsSystem;
    bool m_useCPUPhysics = false; // flag per attivare la fisica CPU

    GLuint m_gpuTimerStart = 0;
    GLuint m_gpuTimerEnd = 0;
    bool m_gpuTimersInitialized = false;

    // Performance e Debug
    std::unique_ptr<PerformanceMonitor> m_performanceMonitor;
    std::unique_ptr<DebugUI> m_debugUI;

    // Timing
    std::chrono::high_resolution_clock::time_point m_lastFrame;
    std::chrono::steady_clock::time_point m_frameStart;
    std::chrono::steady_clock::time_point m_renderStart;
    std::chrono::steady_clock::time_point m_physicsStart;
    float m_deltaTime;
    bool m_treeExploded = false;
    bool m_busExploded = false;
    bool m_phoneBoothExploded = false;
    std::unique_ptr<Character> m_character;
    std::unique_ptr<Model> m_treeModel;       
    std::unique_ptr<Model> m_treeFragments;    
    std::unique_ptr<Model> m_busModel;           
    std::unique_ptr<Model> m_busFragments;       
    std::unique_ptr<Model> m_phoneBoothModel; 
    std::unique_ptr<Model> m_phoneBoothFragments;
    std::unique_ptr<PhysicsSystem> m_treePhysicsSystem;
    std::unique_ptr<PhysicsSystem> m_busPhysicsSystem;
    std::unique_ptr<PhysicsSystem> m_phoneBoothPhysicsSystem;
    Shader m_characterShader;
    PhysicsSystemCPU m_physicsCPU;
    PhysicsSystemCPU m_treePhysicsCPU;
    PhysicsSystemCPU m_busPhysicsCPU;
    PhysicsSystemCPU m_phoneBoothPhysicsCPU;

    AABB m_buildingLocalAABB{}, m_treeLocalAABB{}, m_busLocalAABB{}, m_phoneLocalAABB{}, m_characterLocalAABB{}, m_groundLocalAABB{};
    bool m_drawBounds = false;
    bool m_prevB = false;

    // Stato dell'applicazione
    bool m_isRunning;

    // Tracciamento dei Colpi
    bool m_buildingHit = false;
    bool m_treeHit = false;
    bool m_busHit = false;
    bool m_phoneHit = false;
    CharacterState m_lastCharacterState = CharacterState::IDLE;

    void handleInput();
    void update(float deltaTime);
    void render();

    // Funzioni di Callback 
    void onKeyPressed(int key, int scancode, int action, int mods);
    void onMouseMove(double xpos, double ypos);
    void onFramebufferResize(int width, int height);
    void createFallbackTreePhysics();

public:
    Application();
    ~Application();

    bool initialize();
    void run();
    void cleanup();

    bool isUsingCPUPhysics() const { return m_useCPUPhysics; }
    void setPerformancePhysicsMode(bool useCPU) {
        if (m_performanceMonitor) {
            m_performanceMonitor->setPhysicsMode(useCPU ? "CPU" : "GPU");
        }
    }

    // Getters
    Window* getWindow() const { return m_window.get(); }
    Camera* getCamera() const { return m_camera.get(); }
    Renderer* getRenderer() const { return m_renderer.get(); }
    Skybox* getSkybox() const { return m_skybox.get(); }
    Model* getBuildingModel() const { return m_buildingModel.get(); }
    Model* getFragmentsModel() const { return m_fragmentsModel.get(); }
    StateManager* getStateManager() const { return m_stateManager.get(); }
    InputHandler* getInputHandler() const { return m_inputHandler.get(); }
    PhysicsSystem* getPhysicsSystem() const { return m_physicsSystem.get(); }
    PerformanceMonitor* getPerformanceMonitor() const { return m_performanceMonitor.get(); }
    DebugUI* getDebugUI() const { return m_debugUI.get(); }

    float getDeltaTime() const { return m_deltaTime; }
};