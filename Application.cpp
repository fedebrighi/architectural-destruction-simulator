#include "Application.h"
#include "Camera.h"
#include "Renderer.h"
#include "Skybox.h"
#include "Model.h"
#include "FileUtils.h"
#include "StateManager.h"
#include "InputHandler.h"
#include "PhysicsSystem.h"
#include "PerformanceMonitor.h"
#include "Logger.h"
#include "DebugUI.h"
#include "Character.h"
#include "AnimatedModel.h"
#include "DebugBB.h"
#include <iostream>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Application::Application()
    : m_deltaTime(0.0f), m_isRunning(false), m_treeExploded(false), m_useCPUPhysics(false) {
    m_lastFrame = std::chrono::high_resolution_clock::now();
}

Application::~Application() {
    cleanup();
}

bool Application::initialize() {
    // Create window
    m_window = std::make_unique<Window>(1280, 720, "PROGETTO DI TESI BRIGHI FEDERICO");

    if (!m_window->initialize()) {
        std::cerr << "Failed to initialize window" << std::endl;
        return false;
    }

    //Initialize Performance Monitor
    m_performanceMonitor = std::make_unique<PerformanceMonitor>();
    if (!m_performanceMonitor->initialize()) {
        std::cerr << "Failed to initialize performance monitor" << std::endl;
        return false;
    }

    // Initialize Logger
    Logger& logger = Logger::getInstance();

    // Add console output (colored, INFO level and above)
    logger.addOutput(std::make_unique<ConsoleOutput>(true, LogLevel::INFO));

    // Add file output (all levels, with rotation)
    logger.addOutput(std::make_unique<FileOutput>("explosion_sim.log", LogLevel::TRACE));

    // Create camera (positioned to look at origin)
    m_camera = std::make_unique<Camera>(glm::vec3(2.0f, 2.0f, 15.0f));

    // Create renderer
    m_renderer = std::make_unique<Renderer>();
    if (!m_renderer->initialize()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return false;
    }

    // Initialize Debug UI
    m_debugUI = std::make_unique<DebugUI>();
    if (!m_debugUI->initialize(m_window->getGLFWWindow())) {
        std::cerr << "Failed to initialize debug UI" << std::endl;
        return false;
    }

    // Inizializza GPU timers
    glGenQueries(1, &m_gpuTimerStart);
    glGenQueries(1, &m_gpuTimerEnd);
    m_gpuTimersInitialized = true;

    // Create state manager
    m_stateManager = std::make_unique<StateManager>();
    m_stateManager->initialize();

    // Create input handler
    m_inputHandler = std::make_unique<InputHandler>();
    m_inputHandler->initialize(m_window.get(), m_camera.get(), m_stateManager.get());

    // Create building physics system
    m_physicsSystem = std::make_unique<PhysicsSystem>();
    if (!m_physicsSystem->initialize(m_stateManager.get())) {
        std::cerr << "Failed to initialize building physics system" << std::endl;
        m_physicsSystem.reset();
    }
    else {
        std::cout << "BUILDING PHYSICS SYSTEM INITIALIZED!" << std::endl;
        m_physicsSystem->loadRealisticPreset();
    }

    // Create UI panels
    m_debugUI->addPanel(m_debugUI->createPerformancePanel(m_performanceMonitor.get()));
    m_debugUI->addPanel(m_debugUI->createPhysicsPanel(m_physicsSystem.get()));
    m_debugUI->addPanel(m_debugUI->createLoggerPanel(&logger));
    m_debugUI->addPanel(m_debugUI->createSystemInfoPanel(m_performanceMonitor.get()));
    m_debugUI->addPanel(m_debugUI->createScenePanel());

    auto controlsPanel = m_debugUI->createControlsPanel();
    controlsPanel->addControl("ESC", "Exit application");
    controlsPanel->addControl("E", "Trigger explosion");
    controlsPanel->addControl("R", "Reset simulation");
    controlsPanel->addControl("I", "Show debug info");
    controlsPanel->addControl("F", "Toggle fullscreen");
    controlsPanel->addControl("F1", "Toggle debug UI");
    controlsPanel->addControl("WASD", "Move camera");
    controlsPanel->addControl("Mouse", "Look around");
    controlsPanel->addControl("Arrow Keys", "Move character");
    controlsPanel->addControl("P", "Toggle CPU/GPU physics");

    m_debugUI->addPanel(std::move(controlsPanel));

    // Setup Performance Callbacks
    m_performanceMonitor->setAlertCallback([](const PerformanceAlert& alert) {
        LOG_WARNING("Performance", alert.message);
        });

    // Create skybox
    m_skybox = std::make_unique<Skybox>();
    std::vector<std::string> skyboxFaces = {
        "skybox/posx.jpg",  // Right  (+X)
        "skybox/negx.jpg",  // Left   (-X)  
        "skybox/posy.jpg",  // Top    (+Y)
        "skybox/negy.jpg",  // Bottom (-Y)
        "skybox/posz.jpg",  // Front  (+Z)
        "skybox/negz.jpg"   // Back   (-Z)
    };

    if (!m_skybox->initialize(skyboxFaces)) {
        std::cerr << "Failed to initialize skybox" << std::endl;
        m_skybox.reset();
    }

    // Load building models
    std::cout << "Loading building models..." << std::endl;

    // Try to load intact building
    std::vector<std::string> intactNames = { "skyscraperD.obj" };
    for (const std::string& name : intactNames) {
        if (FileUtils::fileExists(name)) {
            try {
                m_buildingModel = std::make_unique<Model>(name);
                std::cout << "Intact building loaded: " << name << std::endl;
                break;
            }
            catch (const std::exception& e) {
                std::cerr << "Failed to load " << name << ": " << e.what() << std::endl;
            }
        }
    }

    // Try to load building fragments
    std::vector<std::string> fragmentNames = { "dopo2.obj" };
    for (const std::string& name : fragmentNames) {
        if (FileUtils::fileExists(name)) {
            try {
                m_fragmentsModel = std::make_unique<Model>(name);
                std::cout << "Building fragments loaded: " << name << std::endl;
                break;
            }
            catch (const std::exception& e) {
                std::cerr << "Failed to load " << name << ": " << e.what() << std::endl;
            }
        }
    }

    // Load tree models
    std::cout << "Loading tree models..." << std::endl;
    if (FileUtils::fileExists("model.obj")) {
        try {
            m_treeModel = std::make_unique<Model>("model.obj");
            std::cout << "Tree model loaded: natale.obj" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to load natale.obj: " << e.what() << std::endl;
        }
    }

    if (FileUtils::fileExists("pino_dopo.obj")) {
        try {
            m_treeFragments = std::make_unique<Model>("pino_dopo.obj");
            std::cout << "Tree fragments loaded: natale_dopo.obj" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to load natale_dopo.obj: " << e.what() << std::endl;
        }
    }

    // Load bus post models
    std::cout << "Loading bus post models..." << std::endl;
    if (FileUtils::fileExists("bus.obj")) {
        try {
            m_busModel = std::make_unique<Model>("bus.obj");
            std::cout << "Bus post model loaded: bus.obj" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to load bus.obj: " << e.what() << std::endl;
        }
    }

    if (FileUtils::fileExists("bus_dopo.obj")) {
        try {
            m_busFragments = std::make_unique<Model>("bus_dopo.obj");
            std::cout << "Bus fragments loaded: bus_dopo.obj" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to load bus_dopo.obj: " << e.what() << std::endl;
        }
    }

    // Load phone booth models
    std::cout << "Loading phone booth models..." << std::endl;
    if (FileUtils::fileExists("PhoneBooth_01.obj")) {
        try {
            m_phoneBoothModel = std::make_unique<Model>("PhoneBooth_01.obj");
            std::cout << "Phone booth model loaded: PhoneBooth_01.obj" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to load PhoneBooth_01.obj: " << e.what() << std::endl;
        }
    }

    if (FileUtils::fileExists("tel_dopo.obj")) {
        try {
            m_phoneBoothFragments = std::make_unique<Model>("tel_dopo.obj");
            std::cout << "Phone booth fragments loaded: tel_dopo.obj" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to load tel_dopo.obj: " << e.what() << std::endl;
        }
    }

    // Initialize building physics system
    if (m_buildingModel && m_fragmentsModel && m_physicsSystem) {
        std::cout << "Initializing building physics system..." << std::endl;

        std::vector<glm::vec3> fragmentCenters = m_fragmentsModel->getFragmentCenters();
        std::vector<float> fragmentRadii = m_fragmentsModel->getFragmentRadii();

        if (!fragmentCenters.empty() && !fragmentRadii.empty()) {
            m_physicsSystem->setupFragments(fragmentCenters, fragmentRadii);
            std::cout << "Building physics initialized with " << fragmentCenters.size() << " fragments!" << std::endl;

            // Initialize CPU physics system with same data
            m_physicsCPU.initialize(fragmentCenters, fragmentRadii);
            std::cout << "CPU physics system also initialized with " << fragmentCenters.size() << " fragments!" << std::endl;
            m_physicsCPU.setGroundLevel(-3.0f);  // Stesso livello del GPU physics per il palazzo
        }
        else {
            // Fallback
            std::vector<glm::vec3> fallbackCenters;
            std::vector<float> fallbackRadii;
            for (int i = 0; i < 50; ++i) {
                fallbackCenters.push_back(glm::vec3(
                    ((float)rand() / RAND_MAX - 0.5f) * 2.0f,
                    ((float)rand() / RAND_MAX) * 2.0f,
                    ((float)rand() / RAND_MAX - 0.5f) * 2.0f
                ));
                fallbackRadii.push_back(0.2f);
            }
            m_physicsSystem->setupFragments(fallbackCenters, fallbackRadii);
            m_physicsCPU.initialize(fallbackCenters, fallbackRadii);
            std::cout << "Both physics systems initialized with fallback data!" << std::endl;
            // Dopo m_physicsCPU.initialize(fragmentCenters, fragmentRadii);
            m_physicsCPU.setGroundLevel(-3.0f);  // Stesso livello del GPU physics per il palazzo
        }
    }

    // CREATE TREE PHYSICS SYSTEM 
    std::cout << "Creating tree physics system..." << std::endl;
    m_treePhysicsSystem = std::make_unique<PhysicsSystem>();

    if (m_treePhysicsSystem->initialize(nullptr)) { 
        std::cout << "Tree physics system initialized successfully" << std::endl;

        if (m_treeFragments && m_treeFragments->getMeshCount() > 0) {
            std::cout << "Using REAL tree fragments (" << m_treeFragments->getMeshCount() << " meshes)" << std::endl;

            std::vector<glm::vec3> treeFragmentCenters = m_treeFragments->getFragmentCenters();
            std::vector<float> treeFragmentRadii = m_treeFragments->getFragmentRadii();

            // Trasla i frammenti dell'albero alla posizione corretta
            glm::vec3 treeOffset = glm::vec3(8.0f, 5.5f, -3.0f);
            for (auto& center : treeFragmentCenters) {
                center += treeOffset;
            }

            if (!treeFragmentCenters.empty() && !treeFragmentRadii.empty()) {
                m_treePhysicsSystem->setupFragments(treeFragmentCenters, treeFragmentRadii);

                // Inizializza anche CPU physics per l'albero
                m_treePhysicsCPU.initialize(treeFragmentCenters, treeFragmentRadii);

                std::cout << "Tree physics initialized with " << treeFragmentCenters.size()
                    << " fragments (GPU + CPU)!" << std::endl;
                m_treePhysicsCPU.setGroundLevel(-0.85f);
            }
            else {
                std::cout << "Real tree fragments are empty, using fallback..." << std::endl;
                createFallbackTreePhysics();
            }
        }
        else {
            std::cout << "No tree fragments model found (mesh count: "
                << (m_treeFragments ? m_treeFragments->getMeshCount() : -1)
                << "), using fallback..." << std::endl;
            createFallbackTreePhysics();
        }

        m_treePhysicsSystem->loadDisintegrationPreset();
        std::cout << "TREE PHYSICS SYSTEM READY!" << std::endl;
        // Imposta il ground level per l'albero
        if (m_treePhysicsSystem && m_treePhysicsSystem->isInitialized()) {
            m_treePhysicsSystem->setGroundLevel(-0.85f);
            std::cout << "Tree physics ground level set to: -0.85f" << std::endl;
        }
    }
    else {
        std::cerr << "Failed to initialize tree physics system" << std::endl;
        m_treePhysicsSystem.reset();
    }

    // CREATE BUS PHYSICS SYSTEM
    std::cout << "Creating bus physics system..." << std::endl;
    m_busPhysicsSystem = std::make_unique<PhysicsSystem>();

    if (m_busPhysicsSystem->initialize(nullptr)) {
        std::cout << "Bus physics system initialized successfully" << std::endl;

        if (m_busFragments && m_busFragments->getMeshCount() > 0) {
            std::vector<glm::vec3> busFragmentCenters = m_busFragments->getFragmentCenters();
            std::vector<float> busFragmentRadii = m_busFragments->getFragmentRadii();

            const float busScale = 0.05f;                     //  stessa scala del rendering
            const glm::vec3 busOffset(-5.0f, -0.7f, 1.0f);    // stessa posizione del rendering

            for (size_t i = 0; i < busFragmentCenters.size(); ++i) {
                busFragmentCenters[i] = busFragmentCenters[i] * busScale + busOffset;
                busFragmentRadii[i] *= busScale;
            }

            if (!busFragmentCenters.empty() && !busFragmentRadii.empty()) {
                m_busPhysicsSystem->setupFragments(busFragmentCenters, busFragmentRadii);
                m_busPhysicsCPU.initialize(busFragmentCenters, busFragmentRadii);
                std::cout << "Bus physics initialized with " << busFragmentCenters.size() << " fragments!" << std::endl;
                m_busPhysicsCPU.setGroundLevel(-3.0f);
            }
        }
        m_busPhysicsSystem->loadDisintegrationPreset();
        // Imposta il ground level per il bus
        if (m_busPhysicsSystem && m_busPhysicsSystem->isInitialized()) {
            m_busPhysicsSystem->setGroundLevel(-2.0f);
            std::cout << "Bus physics ground level set to: -2.0f" << std::endl;
        }
    }

    // CREATE PHONE BOOTH PHYSICS SYSTEM
    std::cout << "Creating phone booth physics system..." << std::endl;
    m_phoneBoothPhysicsSystem = std::make_unique<PhysicsSystem>();

    if (m_phoneBoothPhysicsSystem->initialize(nullptr)) {
        std::cout << "Phone booth physics system initialized successfully" << std::endl;

        if (m_phoneBoothFragments && m_phoneBoothFragments->getMeshCount() > 0) {
            std::vector<glm::vec3> phoneFragmentCenters = m_phoneBoothFragments->getFragmentCenters();
            std::vector<float> phoneFragmentRadii = m_phoneBoothFragments->getFragmentRadii();

            // Scala che corrisponde al rendering (0.2f, 0.3f, 0.1f)
            float scaleX = 0.2f;
            float scaleY = 0.3f;
            float scaleZ = 0.1f;
            float avgScale = (scaleX + scaleY + scaleZ) / 3.0f;

            // Posizione della cabina telefonica nella scena
            glm::vec3 phoneOffset = glm::vec3(13.0f, 1.0f, -3.0f);

            // Applica scala e poi offset a ogni frammento
            for (size_t i = 0; i < phoneFragmentCenters.size(); i++) {
                phoneFragmentCenters[i].x *= scaleX;
                phoneFragmentCenters[i].y *= scaleY;
                phoneFragmentCenters[i].z *= scaleZ;
                phoneFragmentCenters[i] += phoneOffset;

                // Scala anche i raggi
                phoneFragmentRadii[i] *= avgScale;
            }

            if (!phoneFragmentCenters.empty() && !phoneFragmentRadii.empty()) {
                m_phoneBoothPhysicsSystem->setupFragments(phoneFragmentCenters, phoneFragmentRadii);
                m_phoneBoothPhysicsCPU.initialize(phoneFragmentCenters, phoneFragmentRadii);
                std::cout << "Phone booth physics initialized with " << phoneFragmentCenters.size() << " fragments!" << std::endl;
                m_phoneBoothPhysicsCPU.setGroundLevel(-2.0f);
            }
        }
        m_phoneBoothPhysicsSystem->loadDisintegrationPreset();
        // Imposta il ground level per la cabina telefonica
        if (m_phoneBoothPhysicsSystem && m_phoneBoothPhysicsSystem->isInitialized()) {
            m_phoneBoothPhysicsSystem->setGroundLevel(-2.0f);
            std::cout << "Phone booth physics ground level set to: -0.85f" << std::endl;
        }
    }

    // Setup physics system performance tracking
    if (m_physicsSystem) {
        m_physicsSystem->setDebugUpdateCallback([this](const PhysicsDebugInfo& info) {
            m_performanceMonitor->setPhysicsMetrics(
                info.activeFragments,
                info.activeFragments > 0 ? 0 : info.activeFragments,
                0.0f
            );
            });
    }

    // CALLBACK: State change
    m_stateManager->setStateChangeCallback([this](AppState oldState, AppState newState) {
        std::cout << "Application state change: "
            << StateManager::stateToString(oldState) << " -> "
            << StateManager::stateToString(newState) << std::endl;

        if (oldState == AppState::FRAGMENTS_SIMULATION && newState == AppState::INTACT_BUILDING) {
            if (m_character) {
                std::cout << "Auto-resetting character on state change" << std::endl;
                m_character->SetPosition(glm::vec3(3.0f, -0.8f, 3.0f));
                m_character->ResetToIdle();
                m_character->SetVisible(true);
            }
        }
        });

    // CALLBACK: Building explosion start
    m_stateManager->setExplosionStartCallback([this]() {

        if (m_physicsSystem && m_physicsSystem->isInitialized()) {
            const auto& explosionData = m_stateManager->getExplosionData();

            std::cout << "DEBUG: Explosion parameters - Center: (" << explosionData.center.x
                << ", " << explosionData.center.y << ", " << explosionData.center.z
                << "), Force: " << explosionData.force << ", Radius: " << explosionData.radius << std::endl;

            // Passo tutti i parametri ala CPU
            m_physicsSystem->setExplosionForce(10.0f);
            m_physicsSystem->setExplosionRadius(19.0f);
            m_physicsSystem->startExplosion(explosionData.center);
            m_physicsCPU.startExplosion(explosionData.center, 15.0f, 19.0f);

            m_physicsSystem->setGroundLevel(-3.0f); // Stesso livello del piano di base
            std::cout << "Building physics ground level set to: -0.85f" << std::endl;
        }
        else {
            std::cout << "ERROR: Physics system not initialized!" << std::endl;
            if (m_physicsSystem) {
                std::cout << "ERROR: GPU physics initialized: " << (m_physicsSystem->isInitialized() ? "YES" : "NO") << std::endl;
            }
            else {
                std::cout << "ERROR: GPU physics system is NULL!" << std::endl;
            }
        }

        if (m_character && m_character->GetState() == CharacterState::PUNCHING) {
            std::cout << "Keeping character visible during explosion" << std::endl;
            m_character->SetVisible(true);
        }
        });

    // CALLBACK: Building explosion end
    m_stateManager->setExplosionEndCallback([this]() {
        std::cout << "Application: Building explosion complete!" << std::endl;

        if (m_character) {
            if (!m_character->IsVisible()) {
                std::cout << "Restoring character visibility after explosion" << std::endl;
                m_character->SetVisible(true);
                m_character->SetRenderScale(1.0f);
            }

            if (m_character->GetState() == CharacterState::PUNCHING) {
                std::cout << "Resetting character state after explosion completion" << std::endl;
                m_character->ResetToIdle();
            }
        }
        });

    // Set up input callbacks
    m_inputHandler->setActionCallback(InputAction::EXIT_APPLICATION, [this]() {
        m_isRunning = false;
        });

    // CALLBACK: Reset completo
    m_inputHandler->setActionCallback(InputAction::RESET_EXPLOSION, [this]() {
        if (m_stateManager) {
            std::cout << "Resetting explosion, character, tree, and all physics..." << std::endl;
            m_stateManager->resetExplosion();

            // Reset building physics
            if (m_physicsSystem) {
                m_physicsSystem->resetSimulation();
                m_physicsSystem->setGroundLevel(-0.85f);
                std::cout << "Building physics system reset!" << std::endl;
            }

            // Reset CPU physics
            m_physicsCPU.reset();
            std::cout << "CPU physics system reset!" << std::endl;
            m_physicsCPU.setGroundLevel(-3.0f);

            // Reset tree physics
            if (m_treePhysicsSystem) {
                m_treePhysicsSystem->resetSimulation();
                m_treePhysicsSystem->setGroundLevel(-0.85f);
                m_treePhysicsCPU.reset();
                std::cout << "Tree physics system reset!" << std::endl;
                m_treePhysicsCPU.setGroundLevel(-0.85f);
            }

            // Reset character
            if (m_character) {
                std::cout << "Resetting character..." << std::endl;
                m_character->SetPosition(glm::vec3(3.0f, -0.8f, 3.0f));
                m_character->ResetToIdle();
                m_character->SetVisible(true);
                std::cout << "Character fully reset!" << std::endl;
            }

            // Reset tree state
            m_treeExploded = false;
            std::cout << "Tree state reset to intact!" << std::endl;

            // Reset bus physics
            if (m_busPhysicsSystem) {
                m_busPhysicsSystem->resetSimulation();
                m_busPhysicsSystem->setGroundLevel(-0.85f);
                m_busPhysicsCPU.reset();
                std::cout << "Bus physics system reset!" << std::endl;
                m_busPhysicsCPU.setGroundLevel(-3.0f);
            }

            // Reset phone booth physics
            if (m_phoneBoothPhysicsSystem) {
                m_phoneBoothPhysicsSystem->resetSimulation();
                m_phoneBoothPhysicsSystem->setGroundLevel(-0.85f);
                m_phoneBoothPhysicsCPU.reset();
                std::cout << "Phone booth physics system reset!" << std::endl;
                m_phoneBoothPhysicsCPU.setGroundLevel(-2.0f);
            }

            // Reset object states
            m_busExploded = false;
            m_phoneBoothExploded = false;
            std::cout << "Bus and phone booth states reset to intact!" << std::endl;

            // Reset hit tracking flags
            m_buildingHit = false;
            m_treeHit = false;
            m_busHit = false;
            m_phoneHit = false;
            m_lastCharacterState = CharacterState::IDLE;
            std::cout << "Hit tracking flags reset!" << std::endl;
        }
        });

    // Debug callback
    m_inputHandler->setActionCallback(InputAction::DEBUG_INFO, [this]() {
        if (!m_stateManager) return;

        std::cout << "=== DEBUG INFO ===" << std::endl;
        std::cout << "Current State: " << StateManager::stateToString(m_stateManager->getCurrentState()) << std::endl;
        std::cout << "Raw deltaTime: " << m_deltaTime << " seconds" << std::endl;
        std::cout << "Calculated FPS: " << (1.0f / m_deltaTime) << std::endl;

        if (m_performanceMonitor) {
            const auto& metrics = m_performanceMonitor->getCurrentMetrics();
            std::cout << "Monitor FPS: " << metrics.currentFPS << std::endl;
            std::cout << "Monitor FrameTime: " << metrics.frameTime << "ms" << std::endl;
        }

        if (m_physicsSystem) {
            std::cout << "Building Physics Active: " << (m_physicsSystem->isSimulationActive() ? "YES" : "NO") << std::endl;
            std::cout << "Building Fragments: " << m_physicsSystem->getFragmentCount() << std::endl;
        }

        std::cout << "CPU Physics Fragments: " << m_physicsCPU.getFragmentCount() << std::endl;
        std::cout << "Using CPU Physics: " << (m_useCPUPhysics ? "YES" : "NO") << std::endl;

        if (m_treePhysicsSystem) {
            std::cout << "Tree Physics Active: " << (m_treePhysicsSystem->isSimulationActive() ? "YES" : "NO") << std::endl;
            std::cout << "Tree Fragments: " << m_treePhysicsSystem->getFragmentCount() << std::endl;
        }

        if (m_character) {
            std::cout << "Character loaded: YES" << std::endl;
            std::cout << "Character position: " << m_character->GetPosition().x << ", "
                << m_character->GetPosition().y << ", " << m_character->GetPosition().z << std::endl;
            std::cout << "Character state: " << static_cast<int>(m_character->GetState()) << std::endl;
            std::cout << "Character visible: " << (m_character->IsVisible() ? "YES" : "NO") << std::endl;
            std::cout << "Character cooldown: " << m_character->GetPunchCooldown() << std::endl;
        }
        else {
            std::cout << "Character loaded: NO" << std::endl;
        }

        std::cout << "Tree exploded: " << (m_treeExploded ? "YES" : "NO") << std::endl;
        std::cout << "==================" << std::endl;
        });

    // Set up window callbacks
    m_window->setKeyCallback([this](int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
            if (m_debugUI) {
                m_debugUI->toggleVisibility();
                LOG_INFO("UI", m_debugUI->isVisible() ? "Debug UI shown" : "Debug UI hidden");
            }
            return;
        }

        if (action == GLFW_PRESS) {
            LOG_DEBUGF("Input", "Key pressed: {} (scancode: {})", key, scancode);
        }

        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            std::cout << "ESC pressed - Forcing application exit!" << std::endl;
            m_isRunning = false;
            return;
        }

        if (key == GLFW_KEY_P && action == GLFW_PRESS) {
            m_useCPUPhysics = !m_useCPUPhysics;
            std::cout << "[TOGGLE] Physics method: " << (m_useCPUPhysics ? "CPU" : "GPU") << std::endl;

            // Aggiorna immediatamente il performance monitor
            if (m_performanceMonitor) {
                m_performanceMonitor->setPhysicsMode(m_useCPUPhysics ? "CPU" : "GPU");
            }
        }

        m_inputHandler->onKeyPressed(key, scancode, action, mods);
        });

    m_window->setMouseCallback([this](double xpos, double ypos) {
        m_inputHandler->onMouseMove(xpos, ypos);
        });

    m_window->setFramebufferCallback([this](int width, int height) {
        onFramebufferResize(width, height);
        });

    // Load character shader
    std::cout << "Loading character shaders..." << std::endl;
    if (!m_characterShader.loadFromFiles("character_vertex.glsl", "character_fragment.glsl")) {
        std::cerr << "Failed to load character shaders" << std::endl;
    }
    else {
        std::cout << "Character shaders loaded successfully" << std::endl;
    }

    // Create character
    std::cout << "Loading character..." << std::endl;
    std::vector<std::string> characterFiles = { "mostro.fbx", "Walking_mostro.fbx", "Idle_mostro.fbx", "Punch_mostro.fbx" };

    for (const std::string& fbxFile : characterFiles) {
        if (FileUtils::fileExists(fbxFile)) {
            try {
                m_character = std::make_unique<Character>(fbxFile);
                m_character->SetPosition(glm::vec3(3.0f, -0.8f, 3.0f));
                std::cout << "Character loaded successfully: " << fbxFile << std::endl;
                break;
            }
            catch (const std::exception& e) {
                std::cerr << "Failed to load " << fbxFile << ": " << e.what() << std::endl;
            }
        }
    }

    if (!m_character) {
        std::cout << "No character model found. Tried: ";
        for (const auto& name : characterFiles) {
            std::cout << name << " ";
        }
        std::cout << std::endl;
    }

    // LOCAL AABB
    auto MakeLocalAABB = [](const Model* m) -> AABB {
        glm::vec3 minB, maxB;
        m->getModelMinMax(minB, maxB);
        AABB a;
        a.c = (minB + maxB) * 0.5f;      // centro = (min+max)/2
        a.e = (maxB - minB) * 0.5f;      // half-extents = (max-min)/2
        return a;
        };

    if (m_buildingModel)    m_buildingLocalAABB = MakeLocalAABB(m_buildingModel.get());
    if (m_treeModel)        m_treeLocalAABB = MakeLocalAABB(m_treeModel.get());
    if (m_busModel)         m_busLocalAABB = MakeLocalAABB(m_busModel.get());
    if (m_phoneBoothModel)  m_phoneLocalAABB = MakeLocalAABB(m_phoneBoothModel.get());
    if (m_character && m_character->GetModelPtr())
        m_characterLocalAABB = MakeLocalAABB(m_character->GetModelPtr());

    m_groundLocalAABB.c = glm::vec3(0.0f, 0.0f, 0.0f);     // Centro
    m_groundLocalAABB.e = glm::vec3(1.0f, 1.0f, 1.0f);     // Half-extents: da -1 a +1

    std::cout << "=== LOCAL AABB DEBUG ===" << std::endl;
    if (m_busModel) {
        std::cout << "Bus local AABB - center: (" << m_busLocalAABB.c.x << ", " << m_busLocalAABB.c.y << ", " << m_busLocalAABB.c.z << ")" << std::endl;
        std::cout << "Bus local AABB - extents: (" << m_busLocalAABB.e.x << ", " << m_busLocalAABB.e.y << ", " << m_busLocalAABB.e.z << ")" << std::endl;
    }
    if (m_phoneBoothModel) {
        std::cout << "Phone local AABB - center: (" << m_phoneLocalAABB.c.x << ", " << m_phoneLocalAABB.c.y << ", " << m_phoneLocalAABB.c.z << ")" << std::endl;
        std::cout << "Phone local AABB - extents: (" << m_phoneLocalAABB.e.x << ", " << m_phoneLocalAABB.e.y << ", " << m_phoneLocalAABB.e.z << ")" << std::endl;
    }

    m_isRunning = true;

    LOG_INFO("Application", "Building Explosion Simulation initialized successfully");
    LOG_INFOF("Application", "Window size: {}x{}", m_window->getWidth(), m_window->getHeight());
    LOG_INFO("Application", m_performanceMonitor->getSystemInfo());

    std::cout << "Application initialized successfully" << std::endl;

    return true;
}

void Application::createFallbackTreePhysics() {
    std::cout << "Creating fallback tree physics with procedural fragments..." << std::endl;

    // Crea frammenti procedurali attorno alla posizione dell'albero
    std::vector<glm::vec3> fallbackTreeCenters;
    std::vector<float> fallbackTreeRadii;

    // Crea una griglia 3D di frammenti attorno all'albero
    glm::vec3 treeBase = glm::vec3(8.0f, 1.0f, -3.0f);

    for (int x = -1; x <= 1; x++) {
        for (int y = 0; y <= 3; y++) {
            for (int z = -1; z <= 1; z++) {
                if (x == 0 && z == 0 && y == 0) continue; // Salta il centro

                glm::vec3 offset = glm::vec3(
                    x * 0.4f + ((float)rand() / RAND_MAX - 0.5f) * 0.2f,
                    y * 0.6f + ((float)rand() / RAND_MAX - 0.5f) * 0.1f,
                    z * 0.4f + ((float)rand() / RAND_MAX - 0.5f) * 0.2f
                );

                fallbackTreeCenters.push_back(treeBase + offset);
                fallbackTreeRadii.push_back(0.1f + ((float)rand() / RAND_MAX) * 0.1f);
            }
        }
    }

    if (!fallbackTreeCenters.empty()) {
        m_treePhysicsSystem->setupFragments(fallbackTreeCenters, fallbackTreeRadii);
        std::cout << "Fallback tree physics created with " << fallbackTreeCenters.size() << " fragments!" << std::endl;
    }
}

void Application::run() {
    if (!m_isRunning) {
        std::cerr << "Application not properly initialized" << std::endl;
        return;
    }

    LOG_INFO("Application", "Starting main application loop...");
    LOG_INFOF("Application", "Target FPS: {} | VSync: {}", 60, "Enabled");

    std::cout << "Starting application loop..." << std::endl;

    while (m_isRunning && !m_window->shouldClose()) {
        auto currentFrame = std::chrono::high_resolution_clock::now();
        m_deltaTime = std::chrono::duration<float>(currentFrame - m_lastFrame).count();
        m_lastFrame = currentFrame;

        update(m_deltaTime);
        render();

        m_window->swapBuffers();
        m_window->pollEvents();

        // Log performance warnings
        static float logTimer = 0.0f;
        logTimer += m_deltaTime;
        if (logTimer >= 5.0f) {
            const auto& metrics = m_performanceMonitor->getCurrentMetrics();
            if (metrics.currentFPS < 30.0f) {
                LOG_WARNINGF("Performance", "Low FPS detected: {:.1f}", metrics.currentFPS);
            }
            if (metrics.physicsTime > 10.0f) {
                LOG_WARNINGF("Performance", "High physics time: {:.2f}ms", metrics.physicsTime);
            }
            logTimer = 0.0f;
        }
    }

    LOG_INFO("Application", "Main application loop ended");
    std::cout << "Application loop ended" << std::endl;
}

void Application::cleanup() {
    LOG_INFO("Application", "Starting application cleanup...");

    if (m_performanceMonitor) {
        m_performanceMonitor->exportMetricsToCSV("performance_data.csv");
        LOG_INFO("Performance", "Performance data exported to performance_data.csv");
    }

    m_debugUI.reset();
    m_performanceMonitor.reset();
    m_character.reset();
    m_treeModel.reset();
    m_treeFragments.reset();
    m_treePhysicsSystem.reset();
    m_physicsSystem.reset();
    m_inputHandler.reset();
    m_stateManager.reset();
    m_fragmentsModel.reset();
    m_buildingModel.reset();
    m_skybox.reset();
    m_renderer.reset();
    m_camera.reset();
    m_window.reset();
    m_isRunning = false;

    LOG_INFO("Application", "Application cleanup completed successfully");
    Logger::shutdown();

    std::cout << "Application cleaned up" << std::endl;
}

void Application::handleInput() {
    // Now handled by InputHandler
}

void Application::update(float deltaTime) {
    // Start frame timing
    m_frameStart = std::chrono::steady_clock::now();
    m_performanceMonitor->beginFrame();

    static Capsule s_prevFist;
    static bool s_prevValid = false;

    // EMERGENCY EXIT CHECK
    if (m_window && m_window->isKeyPressed(GLFW_KEY_ESCAPE)) {
        std::cout << "EMERGENCY ESC - Forcing exit!" << std::endl;
        m_isRunning = false;
        return;
    }

    // === DEBUG BOUNDS TOGGLE (tasto B) ===
    {
        bool bPressed = m_window->isKeyPressed(GLFW_KEY_B);
        if (bPressed && !m_prevB) {
            m_drawBounds = !m_drawBounds;
            std::cout << "[DEBUG] Bounding boxes: " << (m_drawBounds ? "ON" : "OFF") << std::endl;
        }
        m_prevB = bPressed;
    }

    // Update systems
    if (m_stateManager) {
        m_stateManager->update(deltaTime);
    }

    if (m_inputHandler) {
        m_inputHandler->update(deltaTime);
    }

    // Track building physics timing
    bool shouldRunBuildingPhysics = false;
    if (m_stateManager) {
        shouldRunBuildingPhysics = m_stateManager->isExploding() || m_stateManager->isFragmented();
    }

    if (shouldRunBuildingPhysics && m_physicsSystem && m_physicsSystem->isInitialized()) {
        // Set physics mode for performance tracking
        if (m_performanceMonitor) {
            m_performanceMonitor->setPhysicsMode(m_useCPUPhysics ? "CPU" : "GPU");
        }

        // Debug output ogni 60 frame
        static int debugCounter = 0;
        debugCounter++;

        if (m_useCPUPhysics) {
            // CPU PHYSICS PATH
            auto cpuStart = std::chrono::high_resolution_clock::now();

            m_physicsCPU.update(deltaTime);

            auto cpuEnd = std::chrono::high_resolution_clock::now();
            float cpuPhysicsTime = std::chrono::duration<float, std::milli>(cpuEnd - cpuStart).count();

            // Report CPU metrics to performance monitor
            int activeFragments = m_physicsCPU.getFragmentCount();
            if (m_performanceMonitor) {
                m_performanceMonitor->setPhysicsMetrics(activeFragments, 0, cpuPhysicsTime);
            }

            if (debugCounter % 60 == 0) {
                std::cout << "DEBUG: Using CPU physics - fragments: " << activeFragments
                    << ", time: " << cpuPhysicsTime << "ms" << std::endl;
            }
        }
        else {
            // GPU PHYSICS PATH
            auto gpuStart = std::chrono::high_resolution_clock::now();

            // DEBUG: Check for OpenGL errors before GPU update
            GLenum error = glGetError();
            if (error != GL_NO_ERROR) {
                std::cout << "OpenGL ERROR before GPU update: " << error << std::endl;
            }

            // DEBUG: Check compute shader limits
            static bool limitsChecked = false;
            if (!limitsChecked) {
                GLint maxWorkGroupSize[3];
                glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxWorkGroupSize[0]);
                glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &maxWorkGroupSize[1]);
                glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &maxWorkGroupSize[2]);

                GLint maxWorkGroupInvocations;
                glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxWorkGroupInvocations);

                std::cout << "=== AMD GPU COMPUTE LIMITS ===" << std::endl;
                std::cout << "Max work group size: " << maxWorkGroupSize[0] << "x" << maxWorkGroupSize[1] << "x" << maxWorkGroupSize[2] << std::endl;
                std::cout << "Max work group invocations: " << maxWorkGroupInvocations << std::endl;
                std::cout << "===============================" << std::endl;
                limitsChecked = true;
            }

            m_physicsSystem->update(deltaTime);

            // DEBUG: Check for OpenGL errors after GPU update
            error = glGetError();
            if (error != GL_NO_ERROR) {
                std::cout << "OpenGL ERROR after GPU update: " << error << std::endl;
            }

            auto gpuEnd = std::chrono::high_resolution_clock::now();
            float gpuPhysicsTime = std::chrono::duration<float, std::milli>(gpuEnd - gpuStart).count();

            // Report GPU metrics to performance monitor
            int activeFragments = m_physicsSystem->getFragmentCount();
            if (m_performanceMonitor) {
                m_performanceMonitor->setPhysicsMetrics(activeFragments, 0, gpuPhysicsTime);
            }

            if (debugCounter % 60 == 0) {
                std::cout << "DEBUG: Using GPU physics - fragments: " << activeFragments
                    << ", time: " << gpuPhysicsTime << "ms" << std::endl;

                // CPU vs GPU comparison
                std::vector<glm::mat4> gpuTransforms = m_physicsSystem->getFragmentTransforms();
                std::vector<glm::mat4> cpuTransforms = m_physicsCPU.getFragmentTransforms();

                if (!gpuTransforms.empty() && !cpuTransforms.empty()) {
                    std::cout << "=== CPU vs GPU COMPARISON ===" << std::endl;
                    for (int i = 0; i < std::min(3, std::min((int)gpuTransforms.size(), (int)cpuTransforms.size())); i++) {
                        glm::vec3 gpuPos = glm::vec3(gpuTransforms[i][3]);
                        glm::vec3 cpuPos = glm::vec3(cpuTransforms[i][3]);

                        bool gpuIsInitial = (glm::length(gpuPos) < 3.0f);
                        bool cpuIsInitial = (glm::length(cpuPos) < 3.0f);
                        std::cout << "  Fragment " << i << " - GPU pos: (" << gpuPos.x << ", " << gpuPos.y << ", " << gpuPos.z << ")" << std::endl;
                        std::cout << "  Fragment " << i << " - CPU pos: (" << cpuPos.x << ", " << cpuPos.y << ", " << cpuPos.z << ")" << std::endl;
                    }
                    std::cout << "============================" << std::endl;
                }
            }
        }
    }
    else {
        if (m_performanceMonitor) {
            m_performanceMonitor->setPhysicsMode("Idle");
            m_performanceMonitor->setPhysicsMetrics(0, 0, 0.0f);
        }

        static int noPhysicsCounter = 0;
        noPhysicsCounter++;
        if (noPhysicsCounter % 120 == 0) { // Ogni 2 secondi
            std::cout << "DEBUG: No physics running - shouldRun: " << (shouldRunBuildingPhysics ? "YES" : "NO")
                << ", physicsSystem: " << (m_physicsSystem ? "EXISTS" : "NULL");
            if (m_physicsSystem) {
                std::cout << ", initialized: " << (m_physicsSystem->isInitialized() ? "YES" : "NO");
            }
            std::cout << std::endl;
        }
    }

    // Update tree physics system
    if (m_treePhysicsSystem && m_treeExploded && m_treePhysicsSystem->isSimulationActive()) {
        if (m_useCPUPhysics) {
            // USO CPU per l'albero
            m_treePhysicsCPU.update(deltaTime);
        }
        else {
            // USO GPU per l'albero
            m_treePhysicsSystem->update(deltaTime);
        }
    }

    // Update bus physics system
    if (m_busPhysicsSystem && m_busExploded && m_busPhysicsSystem->isSimulationActive()) {
        if (m_useCPUPhysics) {
            m_busPhysicsCPU.update(deltaTime);
        }
        else {
            m_busPhysicsSystem->update(deltaTime);
        }
    }

    // Update phone booth physics system
    if (m_phoneBoothPhysicsSystem && m_phoneBoothExploded && m_phoneBoothPhysicsSystem->isSimulationActive()) {
        if (m_useCPUPhysics) {
            m_phoneBoothPhysicsCPU.update(deltaTime);
        }
        else {
            m_phoneBoothPhysicsSystem->update(deltaTime);
        }
    }

    // Update performance monitor
    m_performanceMonitor->update(deltaTime);

    // Update debug UI
    if (m_debugUI) {
        m_debugUI->beginFrame();
    }

    // Update character
    if (m_character) {
        glm::vec3 moveDirection(0.0f);
        bool isMoving = false;

        // Input movimento
        if (m_window->isKeyPressed(GLFW_KEY_UP)) {
            moveDirection.z -= 1.0f;
            isMoving = true;
        }
        if (m_window->isKeyPressed(GLFW_KEY_DOWN)) {
            moveDirection.z += 1.0f;
            isMoving = true;
        }
        if (m_window->isKeyPressed(GLFW_KEY_LEFT)) {
            moveDirection.x -= 1.0f;
            isMoving = true;
        }
        if (m_window->isKeyPressed(GLFW_KEY_RIGHT)) {
            moveDirection.x += 1.0f;
            isMoving = true;
        }

        // Gestisce il movimento
        if (isMoving) {
            float length = glm::length(moveDirection);
            if (length > 0.0001f) {
                moveDirection = glm::normalize(moveDirection);
                m_character->Move(moveDirection, 2.5f, deltaTime);
                // Limita la posizione del personaggio sul piano di base
                glm::vec3 currentPosition = m_character->GetPosition();

                // Limita l'asse Y per mantenere il personaggio sopra il piano
                currentPosition.y = -0.85f;  // Imposta la posizione Y fissa

                // Limita il movimento su X e Z 
                currentPosition.x = glm::clamp(currentPosition.x, -20.0f, 20.0f);
                currentPosition.z = glm::clamp(currentPosition.z, -20.0f, 20.0f);

                m_character->SetPosition(currentPosition);  // Applica la nuova posizione limitata
            }
        }
        else {
            m_character->Move(glm::vec3(0.0f), 0.0f, deltaTime);
        }

        // aggiorno il character PRIMA delle interazioni
        m_character->Update(deltaTime);

        // AABB DEGLI OGGETTI (stesse matrici del rendering) ===
        glm::vec3 buildingPos = glm::vec3(0.0f, -0.9f, 0.0f);
        glm::vec3 treePos = glm::vec3(8.0f, 4.0f, -3.0f);
        glm::vec3 phoneBoothPos = glm::vec3(13.0f, 1.0f, -3.0f);
        glm::vec3 busPos = glm::vec3(-5.0f, -0.7f, 1.0f);

        // Matrici identiche a quelle usate nel rendering
        glm::mat4 buildingMatrix = glm::mat4(1.0f);
        buildingMatrix = glm::translate(buildingMatrix, buildingPos);
        buildingMatrix = glm::rotate(buildingMatrix, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        buildingMatrix = glm::scale(buildingMatrix, glm::vec3(1.2f, 1.6f, 1.2f));

        glm::mat4 treeMatrix = glm::mat4(1.0f);
        treeMatrix = glm::translate(treeMatrix, treePos);
        treeMatrix = glm::scale(treeMatrix, glm::vec3(2.0f, 3.0f, 2.0f));

        glm::mat4 busMatrix = glm::mat4(1.0f);
        busMatrix = glm::translate(busMatrix, busPos);
        busMatrix = glm::scale(busMatrix, glm::vec3(0.05f));

        glm::mat4 phoneMatrix = glm::mat4(1.0f);
        phoneMatrix = glm::translate(phoneMatrix, phoneBoothPos);
        phoneMatrix = glm::scale(phoneMatrix, glm::vec3(0.2f, 0.3f, 0.1f));

        // Calcola AABB world-space
        AABB buildingAABB = ToWorldAABB(m_buildingLocalAABB, buildingMatrix);
        AABB treeAABB = ToWorldAABB(m_treeLocalAABB, treeMatrix);
        AABB busAABB = ToWorldAABB(m_busLocalAABB, busMatrix);
        AABB phoneAABB = ToWorldAABB(m_phoneLocalAABB, phoneMatrix);

        // Per collision detection (punch), crea AABB inflati separati
        AABB busAABBHit = busAABB;
        AABB phoneAABBHit = phoneAABB;
        phoneAABBHit.e *= 2.0f;

        // === PUSH-OUT: evita attraversamento (solo quando NON sta punchando) ===
        if (m_character->GetState() != CharacterState::PUNCHING) {
            AABB charAABB = m_character->ComputeDynamicAABBWorld();

            auto pushOut = [&](const AABB& objAABB) {
                glm::vec3 push;
                if (AABBvsAABB_Resolve(charAABB, objAABB, push)) {
                    m_character->SetPosition(m_character->GetPosition() + push);
                    charAABB = m_character->ComputeDynamicAABBWorld();
                }
                };

            // Applica push-out per oggetti ancora integri
            if (m_stateManager && m_stateManager->isIntact()) pushOut(buildingAABB);
            if (!m_treeExploded) pushOut(treeAABB);
            // Per il bus, usa AABB ridotto per push-out
            if (!m_busExploded) {
                AABB busAABBPushOut = busAABB;
                busAABBPushOut.e *= 0.5f; // Riduci a metà per permettere avvicinamento
                pushOut(busAABBPushOut);
            }
            if (!m_phoneBoothExploded) pushOut(phoneAABB);
        }

        // === LOGICA INTERAZIONI ===
        bool nearBuilding = m_character->IsNearBuilding(glm::vec3(0.0f, 0.0f, 0.0f), 3.0f);
        bool nearTree = m_character->IsNearTree(glm::vec3(8.0f, -0.8f, -3.0f), 3.0f);
        bool nearBus = m_character->IsNearBuilding(glm::vec3(-5.0f, -0.7f, 1.0f), 3.7f);  // raggio normale
        bool nearPhoneBooth = m_character->IsNearBuilding(glm::vec3(13.0f, 1.0f, -3.0f), 3.0f);  // correggi Y e aumenta raggio

        // === START DEL PUNCH: se cammina ed è vicino a un target, avvia l'animazione ===
        if (m_character->GetState() == CharacterState::WALKING) {
            if (nearBuilding && m_stateManager && m_stateManager->isIntact()) {
                std::cout << "Character near intact building - starting punch!" << std::endl;
                m_character->InteractWithBuilding();
            }
            else if (nearTree && !nearBuilding && !m_treeExploded) {
                m_character->InteractWithTree();
            }
            else if (nearBus && !m_busExploded && !nearBuilding && !nearTree && !nearPhoneBooth) {
                // Per il bus, controlla la distanza effettiva e inizia il punch prima
                float distToBus = glm::length(m_character->GetPosition() - glm::vec3(-5.0f, -0.7f, 1.0f));
                if (distToBus < 4.5f) {  // Aumenta la distanza di trigger per il bus
                    m_character->InteractWithBuilding();
                }
            }
            else if (nearPhoneBooth && !m_phoneBoothExploded && !nearBuilding && !nearTree && !nearBus) {
                m_character->InteractWithBuilding();
            }
        }

        // === CONTROLLO COLLISIONE DURANTE IL PUNCH (ESPLOSIONE IMMEDIATA) ===
        if (m_character->GetState() == CharacterState::PUNCHING) {
            // Reset flags quando inizia un nuovo punch
            if (m_lastCharacterState != CharacterState::PUNCHING) {
                m_buildingHit = m_treeHit = m_busHit = m_phoneHit = false;
            }
            m_lastCharacterState = CharacterState::PUNCHING;

            Capsule fist = m_character->GetRightFistCapsuleWorld();
            fist.r = 0.5f;
            glm::vec3 hitPoint;

            // PALAZZO - controllo immediato durante il punch
            if (nearBuilding && m_stateManager && m_stateManager->isIntact() && !m_buildingHit) {
                if (CapsuleVsAABB(fist, buildingAABB, hitPoint)) {
                    std::cout << "BUILDING HIT! Triggering immediate explosion!" << std::endl;
                    m_stateManager->triggerExplosion();
                    m_buildingHit = true;
                }
            }

            // ALBERO - controllo immediato durante il punch
            if (nearTree && !nearBuilding && !m_treeExploded && !m_treeHit) {
                if (CapsuleVsAABB(fist, treeAABB, hitPoint)) {
                    std::cout << "TREE HIT! Triggering immediate explosion!" << std::endl;
                    m_treeExploded = true;

                    if (m_treePhysicsSystem) {
                        glm::vec3 treeExplosionCenter = glm::vec3(8.0f, 5.5f, -3.0f);
                        float treeForce = 7.0f;
                        float treeRadius = 8.0f;

                        m_treePhysicsSystem->setExplosionForce(treeForce);
                        m_treePhysicsSystem->setExplosionRadius(treeRadius);
                        m_treePhysicsSystem->startExplosion(treeExplosionCenter);
                        m_treePhysicsCPU.startExplosion(treeExplosionCenter, treeForce, treeRadius);
                    }
                    m_treeHit = true;
                }
            }

            
            // BUS - controllo immediato durante il punch
            if (nearBus && !m_busExploded && !nearBuilding && !nearTree && !nearPhoneBooth && !m_busHit) {
               
                float distance = glm::length(fist.a - busAABB.c);
                float threshold = 3.0f;  // Soglia di distanza per il rilevamento

                // Aggiungi una condizione per eseguire la collisione solo se la distanza è inferiore alla soglia
                if (distance < threshold) {
                    std::cout << "Verifica collisione con il bus: " << busPos.x << ", " << busPos.y << ", " << busPos.z << std::endl;
                    std::cout << "Posizione del pugno: " << fist.a.x << ", " << fist.a.y << ", " << fist.a.z << std::endl;
                    std::cout << "AABB del bus: centro: " << busAABB.c.x << ", " << busAABB.c.y << ", " << busAABB.c.z << std::endl;
                    std::cout << "AABB inflato del bus: centro: " << busAABBHit.c.x << ", " << busAABBHit.c.y << ", " << busAABBHit.c.z << std::endl;
                    std::cout << "Distance between fist and bus: " << distance << std::endl;
                    std::cout << "Bus AABB center: " << busAABB.c.x << ", " << busAABB.c.y << ", " << busAABB.c.z << std::endl;
                    std::cout << "Fist position: " << fist.a.x << ", " << fist.a.y << ", " << fist.a.z << std::endl;

                    AABB busAABBHitNormal = busAABB;
                    if (CapsuleVsAABB(fist, busAABBHitNormal, hitPoint)) {
                        std::cout << "BUS HIT! Triggering immediate explosion!" << std::endl;

                        // === PUSH-BACK: Applica una spinta al personaggio PRIMA dell'esplosione ===
                        glm::vec3 pushDirection = m_character->GetPosition() - busAABB.c; // Direzione per spingere via dal bus
                        float pushStrength = 0.5f;  // Forza della spinta
                        pushDirection = glm::normalize(pushDirection);  // Normalizza per evitare distorsioni
                        m_character->SetPosition(m_character->GetPosition() + pushDirection * pushStrength);  // Spingi il personaggio indietro

                        // Attivo l'esplosione del bus dopo aver applicato la spinta
                        m_busExploded = true;

                        if (m_busPhysicsSystem) {
                            m_busPhysicsSystem->startExplosion(busPos);
                            m_busPhysicsCPU.startExplosion(busPos, 10.0f, 4.0f);
                        }

                        m_busHit = true;
                    }
                }
                else {
                    std::cout << "Pugno troppo lontano dal bus, ignorando la collisione." << std::endl;
                }
            }


            // PHONE BOOTH - controllo immediato durante il punch
            if (nearPhoneBooth && !m_phoneBoothExploded && !nearBuilding && !nearTree && !nearBus && !m_phoneHit) {
                if (CapsuleVsAABB(fist, phoneAABBHit, hitPoint)) {
                    std::cout << "PHONE BOOTH HIT! Triggering immediate explosion!" << std::endl;
                    m_phoneBoothExploded = true;

                    if (m_phoneBoothPhysicsSystem) {
                        m_phoneBoothPhysicsSystem->startExplosion(phoneBoothPos);
                        m_phoneBoothPhysicsCPU.startExplosion(phoneBoothPos, 6.0f, 5.0f);
                    }
                    m_phoneHit = true;
                }
            }
        }
        else {
            // Quando non sta più punchando, aggiorna lo stato
            m_lastCharacterState = m_character->GetState();
        }

        // === RESET QUANDO FINISCE IL PUNCH ===
        if (m_character->HasFinishedPunching()) {
            m_character->ResetToIdle();
        }

        // GESTIONE STATO POST-ESPLOSIONE
        if (m_stateManager) {
            if (m_stateManager->isFragmented() && !m_character->IsVisible()) {
                std::cout << "Fixing character visibility after explosion" << std::endl;
                m_character->SetVisible(true);
                m_character->ResetToIdle();
            }

            if (m_stateManager->isFragmented() &&
                m_character->GetState() == CharacterState::PUNCHING &&
                m_character->GetPunchCooldown() <= 0.0f) {

                std::cout << "Force-resetting character state after explosion" << std::endl;
                m_character->ResetToIdle();
            }
        }
    }

    // Update other systems based on current state
    if (m_stateManager) {
        switch (m_stateManager->getCurrentState()) {
        case AppState::INTACT_BUILDING:
            // Update intact building state
            break;

        case AppState::EXPLOSION_TRANSITION:
            // Update explosion transition
            break;

        case AppState::FRAGMENTS_SIMULATION:
            // Update fragments simulation
            break;

        case AppState::PAUSED:
            // Don't update physics when paused
            return;

        default:
            break;
        }
    }
}

void Application::render() {
    // Start render timing
    m_renderStart = std::chrono::steady_clock::now();

    // Clear screen
    m_renderer->clear(glm::vec3(0.0f, 0.0f, 0.0f));

    // Begin scene
    m_renderer->beginScene(m_camera.get(), m_window->getWidth(), m_window->getHeight());

    // Get matrices
    glm::mat4 view = m_camera->getViewMatrix();
    glm::mat4 projection = m_camera->getProjectionMatrix(m_window->getAspectRatio());

    // Render skybox first (will be behind everything)
    if (m_skybox && m_skybox->isValid()) {
        m_skybox->render(view, projection);
    }

    // GROUND PLANE
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glm::mat4 groundModel = glm::mat4(1.0f);
    groundModel = glm::translate(groundModel, glm::vec3(0.0f, -0.85f, 0.0f));
    groundModel = glm::scale(groundModel, glm::vec3(50.0f, 0.5f, 50.0f));
    m_renderer->renderCube(groundModel, view, projection, glm::vec3(0.3f, 0.3f, 0.4f));
    glEnable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    // BUILDING: Render building based on current state
    glm::mat4 buildingModelMatrix = glm::mat4(1.0f);
    buildingModelMatrix = glm::translate(buildingModelMatrix, glm::vec3(0.0f, -0.9f, 0.0f));
    buildingModelMatrix = glm::rotate(buildingModelMatrix, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    buildingModelMatrix = glm::scale(buildingModelMatrix, glm::vec3(1.2f, 1.6f, 1.2f));

    if (m_stateManager) {
        if (m_stateManager->isIntact()) {
            // INTACT BUILDING RENDERING
            if (m_buildingModel) {
                Shader* shader = m_renderer->getBasicShader();
                if (shader) {
                    shader->use();
                    shader->setMat4("model", buildingModelMatrix);
                    shader->setMat4("view", view);
                    shader->setMat4("projection", projection);

                    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(buildingModelMatrix)));
                    shader->setMat3("normalMatrix", normalMatrix);
                    shader->setBool("useObjectColor", false);
                    shader->setBool("isFragment", false);
                    shader->setFloat("explosionFactor", 0.0f);

                    m_buildingModel->render(*shader);
                }
            }
        }
        else {
            // FRAGMENTS RENDERING - SEMPRE SE NON È INTATTO OPPURE SE LA FISICA È ATTIVA
            static std::vector<glm::mat4> s_cachedGPUTransforms;
            static std::vector<glm::mat4> s_cachedCPUTransforms;
            static int s_tfCounter = 0;
            static int s_tfPeriod = 10;  // prova 10–20

            bool shouldRenderFragments = !m_stateManager->isIntact() ||
                (m_physicsSystem && m_physicsSystem->isSimulationActive());

            if (shouldRenderFragments && m_physicsSystem && m_physicsSystem->isInitialized()) {

                // --- CACHE DELLE TRASFORMAZIONI (no readback ogni frame) ---
                std::vector<glm::mat4> dynamicTransforms;

                if (m_useCPUPhysics) {
                    if (s_cachedCPUTransforms.empty() || (s_tfCounter % s_tfPeriod) == 0) {
                        s_cachedCPUTransforms = m_physicsCPU.getFragmentTransforms();
                    }
                    dynamicTransforms = s_cachedCPUTransforms;
                }
                else {
                    if (s_cachedGPUTransforms.empty() || (s_tfCounter % s_tfPeriod) == 0) {
                        s_cachedGPUTransforms = m_physicsSystem->getFragmentTransforms();
                    }
                    dynamicTransforms = s_cachedGPUTransforms;
                }
                ++s_tfCounter;

                // --- DRAW ---
                if (!dynamicTransforms.empty() && m_fragmentsModel &&
                    dynamicTransforms.size() >= m_fragmentsModel->getMeshCount())
                {
                    Shader* shader = m_renderer->getBasicShader();
                    if (shader) {
                        shader->use();
                        shader->setMat4("view", view);
                        shader->setMat4("projection", projection);

                        shader->setBool("isFragment", true);
                        shader->setFloat("explosionFactor", 0.3f);
                        shader->setFloat("time", (float)glfwGetTime());

                        const size_t meshCount = m_fragmentsModel->getMeshCount();
                        for (size_t i = 0; i < meshCount; ++i) {
                            glm::mat4 fragmentTransform = dynamicTransforms[i];
                            fragmentTransform = glm::translate(fragmentTransform, glm::vec3(0.0f, -0.75f, 0.0f));
                            glm::mat4 scaledTransform = glm::scale(fragmentTransform, glm::vec3(0.6f, 0.6f, 0.6f));

                            shader->setMat4("model", scaledTransform);
                            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(scaledTransform)));
                            shader->setMat3("normalMatrix", normalMatrix);

                            if (Mesh* mesh = m_fragmentsModel->getMesh(i)) {
                                mesh->render(*shader);
                            }
                        }
                    }
                }
            }

            else {
                // No physics system - fallback
                buildingModelMatrix = glm::scale(buildingModelMatrix, glm::vec3(1.0f, 2.0f, 1.0f));
                m_renderer->renderCube(buildingModelMatrix, view, projection, glm::vec3(1.0f, 0.2f, 0.1f));
            }
        }
    }

    // Render character
    if (m_character && m_characterShader.isValid()) {
        m_characterShader.use();
        m_characterShader.setMat4("view", view);
        m_characterShader.setMat4("projection", projection);

        // Setup lighting for character
        m_characterShader.setVec3("lightPos", glm::vec3(2.0f, 4.0f, 2.0f));
        m_characterShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
        m_characterShader.setVec3("viewPos", m_camera->getPosition());
        m_characterShader.setBool("hasTexture", true);

        m_character->Render(m_characterShader);
    }

    // RENDER TREES
    if (m_treeModel) {
        glm::mat4 treeMatrix = glm::mat4(1.0f);
        treeMatrix = glm::translate(treeMatrix, glm::vec3(8.0f, 4.0f, -3.0f));
        treeMatrix = glm::scale(treeMatrix, glm::vec3(2.0f, 3.0f, 2.0f));

        if (!m_treeExploded) {
            // ALBERO INTATTO
            Shader* shader = m_renderer->getBasicShader();
            if (shader) {
                shader->use();
                shader->setMat4("model", treeMatrix);
                shader->setMat4("view", view);
                shader->setMat4("projection", projection);

                glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(treeMatrix)));
                shader->setMat3("normalMatrix", normalMatrix);
                shader->setBool("useObjectColor", false);
                shader->setBool("isFragment", false);
                shader->setFloat("explosionFactor", 0.0f);

                m_treeModel->render(*shader);
            }
        }
        else {
            // ALBERO ESPLOSO
            if (m_treeFragments && m_treePhysicsSystem && m_treePhysicsSystem->isInitialized()) {

                std::vector<glm::mat4> treeTransforms;

                if (m_useCPUPhysics) {
                    // USA CPU PHYSICS per l'albero
                    treeTransforms = m_treePhysicsCPU.getFragmentTransforms();

                    // Debug opzionale per vedere quale sistema sta usando
                    static int cpuDebugCounter = 0;
                    if (++cpuDebugCounter % 120 == 0) {
                        std::cout << "🌳 Tree rendering using CPU physics" << std::endl;
                    }
                }
                else {
                    treeTransforms = m_treePhysicsSystem->getFragmentTransforms();

                    static int gpuDebugCounter = 0;
                    if (++gpuDebugCounter % 120 == 0) { 
                        std::cout << "🌳 Tree rendering using GPU physics" << std::endl;
                    }
                }

                if (!treeTransforms.empty() && m_treeFragments->getMeshCount() > 0) {
                    Shader* shader = m_renderer->getBasicShader();
                    if (shader) {
                        shader->use();
                        shader->setMat4("view", view);
                        shader->setMat4("projection", projection);
                        shader->setBool("isFragment", true);  // È un frammento in esplosione
                        shader->setFloat("explosionFactor", 0.2f); // Effetto esplosione per albero (più leggero)
                        shader->setFloat("time", (float)glfwGetTime());

                        // Render ogni frammento reale dell'albero
                        size_t meshCount = m_treeFragments->getMeshCount();
                        size_t maxRender = std::min(meshCount, treeTransforms.size());

                        for (size_t i = 0; i < maxRender; ++i) {
                            glm::mat4 fragmentTransform = treeTransforms[i];

                            // Applica la scala dell'albero ai frammenti
                            glm::mat4 scaledTransform = glm::scale(fragmentTransform, glm::vec3(1.5f, 2.0f, 1.5f));

                            shader->setMat4("model", scaledTransform);

                            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(scaledTransform)));
                            shader->setMat3("normalMatrix", normalMatrix);

                            Mesh* mesh = m_treeFragments->getMesh(i);
                            if (mesh) {
                                mesh->render(*shader);
                            }
                        }
                    }
                }
            }
        }

        // RENDER BUS
        if (m_busModel) {
            glm::mat4 busMatrix = glm::mat4(1.0f);
            busMatrix = glm::translate(busMatrix, glm::vec3(-5.0f, -0.7f, 1.0f));
            busMatrix = glm::scale(busMatrix, glm::vec3(0.05f));

            if (!m_busExploded) {
                // BUS INTATTO
                Shader* shader = m_renderer->getBasicShader();
                if (shader) {
                    shader->use();
                    shader->setMat4("model", busMatrix);
                    shader->setMat4("view", view);
                    shader->setMat4("projection", projection);

                    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(busMatrix)));
                    shader->setMat3("normalMatrix", normalMatrix);
                    shader->setBool("useObjectColor", false);
                    shader->setBool("isFragment", false);
                    shader->setFloat("explosionFactor", 0.0f);

                    m_busModel->render(*shader);
                }
            }
            else {
                // BUS ESPLOSO
                if (m_busFragments && m_busPhysicsSystem && m_busPhysicsSystem->isInitialized()) {
                    std::vector<glm::mat4> busTransforms;

                    if (m_useCPUPhysics) {
                        busTransforms = m_busPhysicsCPU.getFragmentTransforms();
                    }
                    else {
                        busTransforms = m_busPhysicsSystem->getFragmentTransforms();
                    }

                    if (!busTransforms.empty() && m_busFragments->getMeshCount() > 0) {
                        Shader* shader = m_renderer->getBasicShader();
                        if (shader) {
                            shader->use();
                            shader->setMat4("view", view);
                            shader->setMat4("projection", projection);
                            shader->setBool("isFragment", true);
                            shader->setFloat("explosionFactor", 0.15f);
                            shader->setFloat("time", (float)glfwGetTime());

                            size_t meshCount = m_busFragments->getMeshCount();
                            size_t maxRender = std::min(meshCount, busTransforms.size());

                            for (size_t i = 0; i < maxRender; ++i) {
                                glm::mat4 fragmentTransform = busTransforms[i];
                                glm::mat4 scaledTransform = glm::scale(fragmentTransform, glm::vec3(0.02f, 0.02f, 0.02f)); 
                                shader->setMat4("model", scaledTransform);

                                glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(scaledTransform)));
                                shader->setMat3("normalMatrix", normalMatrix);

                                Mesh* mesh = m_busFragments->getMesh(i);
                                if (mesh) {
                                    mesh->render(*shader);
                                }
                            }
                        }
                    }
                }
            }
        }

        // RENDER PHONE BOOTH
        if (m_phoneBoothModel) {
            glm::mat4 phoneMatrix = glm::mat4(1.0f);
            phoneMatrix = glm::translate(phoneMatrix, glm::vec3(13.0f, 1.0f, -3.0f));
            phoneMatrix = glm::scale(phoneMatrix, glm::vec3(0.2f, 0.3f, 0.1f));

            if (!m_phoneBoothExploded) {
                // CABINA TELEFONICA INTATTA
                Shader* shader = m_renderer->getBasicShader();
                if (shader) {
                    shader->use();
                    shader->setMat4("model", phoneMatrix);
                    shader->setMat4("view", view);
                    shader->setMat4("projection", projection);

                    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(phoneMatrix)));
                    shader->setMat3("normalMatrix", normalMatrix);
                    shader->setBool("useObjectColor", false);
                    shader->setBool("isFragment", false);
                    shader->setFloat("explosionFactor", 0.0f);

                    m_phoneBoothModel->render(*shader);
                }
            }
            else {
                // CABINA TELEFONICA ESPLOSA
                if (m_phoneBoothFragments && m_phoneBoothPhysicsSystem && m_phoneBoothPhysicsSystem->isInitialized()) {
                    std::vector<glm::mat4> phoneTransforms;

                    if (m_useCPUPhysics) {
                        phoneTransforms = m_phoneBoothPhysicsCPU.getFragmentTransforms();
                    }
                    else {
                        phoneTransforms = m_phoneBoothPhysicsSystem->getFragmentTransforms();
                    }

                    if (!phoneTransforms.empty() && m_phoneBoothFragments->getMeshCount() > 0) {
                        Shader* shader = m_renderer->getBasicShader();
                        if (shader) {
                            shader->use();
                            shader->setMat4("view", view);
                            shader->setMat4("projection", projection);
                            shader->setBool("isFragment", true);
                            shader->setFloat("explosionFactor", 0.2f);
                            shader->setFloat("time", (float)glfwGetTime());

                            size_t meshCount = m_phoneBoothFragments->getMeshCount();
                            size_t maxRender = std::min(meshCount, phoneTransforms.size());

                            for (size_t i = 0; i < maxRender; ++i) {
                                glm::mat4 fragmentTransform = phoneTransforms[i];
                                glm::mat4 scaledTransform = glm::scale(fragmentTransform, glm::vec3(0.15f, 0.2f, 0.12f));

                                shader->setMat4("model", scaledTransform);

                                glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(scaledTransform)));
                                shader->setMat3("normalMatrix", normalMatrix);

                                Mesh* mesh = m_phoneBoothFragments->getMesh(i);
                                if (mesh) {
                                    mesh->render(*shader);
                                }
                            }
                        }
                    }
                }
            }
        }

        // === DEBUG: DRAW WORLD-SPACE AABBs ===
        if (m_drawBounds) {

            // PALAZZO (identica a quella usata sopra)
            glm::mat4 buildingM = glm::mat4(1.0f);
            buildingM = glm::translate(buildingM, glm::vec3(0.0f, -0.9f, 0.0f));
            buildingM = glm::rotate(buildingM, glm::radians(180.0f), glm::vec3(0, 1, 0));
            buildingM = glm::scale(buildingM, glm::vec3(1.2f, 1.6f, 1.2f));

            // ALBERO (identica a quella usata sopra)
            glm::mat4 treeM = glm::mat4(1.0f);
            treeM = glm::translate(treeM, glm::vec3(8.0f, 4.0f, -3.0f));
            treeM = glm::scale(treeM, glm::vec3(2.0f, 3.0f, 2.0f));

            // BUS (identica)
            glm::mat4 busM = glm::mat4(1.0f);
            busM = glm::translate(busM, glm::vec3(-5.0f, -0.7f, 1.0f));
            busM = glm::scale(busM, glm::vec3(0.05f, 0.05f, 0.05f));

            // CABINA (identica)
            glm::mat4 phoneM = glm::mat4(1.0f);
            phoneM = glm::translate(phoneM, glm::vec3(13.0f, 1.0f, -3.0f));
            phoneM = glm::scale(phoneM, glm::vec3(0.2f, 0.3f, 0.1f));

            //PIANO
            glm::mat4 groundM = glm::mat4(1.0f);
            groundM = glm::translate(groundM, glm::vec3(0.0f, -0.85f, 0.0f));
            groundM = glm::scale(groundM, glm::vec3(50.0f, 0.1f, 50.0f));

            auto rc = [&](const glm::mat4& M, const glm::vec3& color, const AABB& local) {
                if (local.e.x <= 0.0f && local.e.y <= 0.0f && local.e.z <= 0.0f) return;
                AABB w = ToWorldAABB(local, M);
                DrawAABB_Wire(w, color, view, projection,
                    [&](const glm::mat4& MM, const glm::mat4& V, const glm::mat4& P, const glm::vec3& C) {
                        m_renderer->renderCube(MM, V, P, C);
                    });
                };

            if (m_buildingModel && m_stateManager && m_stateManager->isIntact())   rc(buildingM, glm::vec3(0, 1, 0), m_buildingLocalAABB);    // verde
            if (m_treeModel && !m_treeExploded)       rc(treeM, glm::vec3(1, 1, 0), m_treeLocalAABB);        // giallo
            if (m_busModel && !m_busExploded)        rc(busM, glm::vec3(0, 1, 1), m_busLocalAABB);         // ciano
            if (m_phoneBoothModel && !m_phoneBoothExploded) rc(phoneM, glm::vec3(1, 0, 1), m_phoneLocalAABB);       // magenta

            AABB groundWorldAABB;
            groundWorldAABB.c = glm::vec3(0.0f, -0.85f, 0.0f);     // Centro del piano
            groundWorldAABB.e = glm::vec3(25.0f, 0.25f, 25.0f);    // Half-extents: 50/2, 0.1/2, 50/2

            DrawAABB_Wire(groundWorldAABB, glm::vec3(1.0f, 1.0f, 1.0f), view, projection,
                [&](const glm::mat4& MM, const glm::mat4& V, const glm::mat4& P, const glm::vec3& C) {
                    m_renderer->renderCube(MM, V, P, C);
                });

            // Personaggio: usa la sua model matrix reale
            if (m_character) {
                AABB charW = m_character->ComputeDynamicAABBWorld();   // <-- world-space AABB
                DrawAABB_Wire(
                    charW, glm::vec3(1, 0, 0),                           // rosso
                    view, projection,
                    [&](const glm::mat4& MM, const glm::mat4& V, const glm::mat4& P, const glm::vec3& C) {
                        m_renderer->renderCube(MM, V, P, C);
                    }
                );
            }
        }

        // Render debug UI
        if (m_debugUI && m_debugUI->isVisible()) {
            m_debugUI->render();
        }

        // End scene
        m_renderer->endScene();

        // End render timing
        auto renderEnd = std::chrono::steady_clock::now();
        float renderTime = std::chrono::duration<float, std::milli>(renderEnd - m_renderStart).count();
        m_performanceMonitor->setRenderTime(renderTime);

        // End frame timing
        m_performanceMonitor->endFrame();

        // Complete debug UI frame
        if (m_debugUI) {
            m_debugUI->endFrame();
        }
    }

}
// Callback implementations
void Application::onKeyPressed(int key, int scancode, int action, int mods) {
    // Now handled by InputHandler
}

void Application::onMouseMove(double xpos, double ypos) {
    // Now handled by InputHandler
}

void Application::onFramebufferResize(int width, int height) {
    std::cout << "Framebuffer resized to: " << width << "x" << height << std::endl;
    if (m_renderer) {
        m_renderer->setViewport(width, height);
    }
}