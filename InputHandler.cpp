#include "InputHandler.h"
#include "Window.h"
#include "Camera.h"
#include "StateManager.h"
#include <iostream>
#include <GLFW/glfw3.h>
#include <algorithm>

InputHandler::InputHandler()
    : m_window(nullptr), m_camera(nullptr), m_stateManager(nullptr), m_cameraEnabled(true) {
}

InputHandler::~InputHandler() {
}

void InputHandler::initialize(Window* window, Camera* camera, StateManager* stateManager) {
    m_window = window;
    m_camera = camera;
    m_stateManager = stateManager;

    setupDefaultKeyBindings();
    setupCallbacks();

    std::cout << "InputHandler initialized" << std::endl;
    printKeyBindings();
}

void InputHandler::setupDefaultKeyBindings() {
    m_keyBindings.clear();

    addKeyBinding(GLFW_KEY_W, InputAction::MOVE_FORWARD, false);
    addKeyBinding(GLFW_KEY_S, InputAction::MOVE_BACKWARD, false);
    addKeyBinding(GLFW_KEY_A, InputAction::MOVE_LEFT, false);
    addKeyBinding(GLFW_KEY_D, InputAction::MOVE_RIGHT, false);
    addKeyBinding(GLFW_KEY_SPACE, InputAction::MOVE_UP, false);
    addKeyBinding(GLFW_KEY_LEFT_SHIFT, InputAction::MOVE_DOWN, false);

    addKeyBinding(GLFW_KEY_C, InputAction::TOGGLE_CAMERA, true);
    addKeyBinding(GLFW_KEY_F, InputAction::TOGGLE_FULLSCREEN, true);

    addKeyBinding(GLFW_KEY_E, InputAction::TRIGGER_EXPLOSION, true);
    addKeyBinding(GLFW_KEY_R, InputAction::RESET_EXPLOSION, true);
    addKeyBinding(GLFW_KEY_ESCAPE, InputAction::EXIT_APPLICATION, true);

    addKeyBinding(GLFW_KEY_I, InputAction::DEBUG_INFO, true);
}

void InputHandler::setupCallbacks() {
    setActionCallback(InputAction::TOGGLE_CAMERA, [this]() {
        toggleCameraControl();
        });

    setActionCallback(InputAction::TOGGLE_FULLSCREEN, [this]() {
        if (m_window) {
            std::cout << "Toggling fullscreen..." << std::endl;
            m_window->toggleFullscreen();
        }
        });

    setActionCallback(InputAction::TRIGGER_EXPLOSION, [this]() {
        if (m_stateManager) {
            std::cout << "Triggering explosion..." << std::endl;
            m_stateManager->triggerExplosion();
        }
        });

    setActionCallback(InputAction::RESET_EXPLOSION, [this]() {
        if (m_stateManager) {
            std::cout << "Resetting explosion..." << std::endl;
            m_stateManager->resetExplosion();
        }
        });

    setActionCallback(InputAction::DEBUG_INFO, [this]() {
        if (m_stateManager) {
            std::cout << "=== DEBUG INFO ===" << std::endl;
            std::cout << "Current State: " << StateManager::stateToString(m_stateManager->getCurrentState()) << std::endl;
            std::cout << "Camera Enabled: " << (m_cameraEnabled ? "Yes" : "No") << std::endl;
            std::cout << "Explosion Active: " << (m_stateManager->isExplosionActive() ? "Yes" : "No") << std::endl;
            if (m_stateManager->isExplosionActive()) {
                const auto& explosion = m_stateManager->getExplosionData();
                std::cout << "Explosion Progress: " << (explosion.getProgress() * 100.0f) << "%" << std::endl;
            }
            std::cout << "==================" << std::endl;
        }
        });
}

void InputHandler::update(float deltaTime) {
    m_justPressedKeys.clear();
    m_justReleasedKeys.clear();

    processInput(deltaTime);
}

void InputHandler::processInput(float deltaTime) {
    if (m_cameraEnabled && m_camera && m_stateManager && !m_stateManager->isPaused()) {
        if (isKeyPressed(GLFW_KEY_W)) {
            m_camera->processKeyboard(FORWARD, deltaTime);
        }
        if (isKeyPressed(GLFW_KEY_S)) {
            m_camera->processKeyboard(BACKWARD, deltaTime);
        }
        if (isKeyPressed(GLFW_KEY_A)) {
            m_camera->processKeyboard(LEFT, deltaTime);
        }
        if (isKeyPressed(GLFW_KEY_D)) {
            m_camera->processKeyboard(RIGHT, deltaTime);
        }
        if (isKeyPressed(GLFW_KEY_SPACE)) {
            m_camera->processKeyboard(UP, deltaTime);
        }
        if (isKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
            m_camera->processKeyboard(DOWN, deltaTime);
        }
    }
}

void InputHandler::onKeyPressed(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        m_pressedKeys.insert(key);
        m_justPressedKeys.insert(key);

        for (const auto& binding : m_keyBindings) {
            if (binding.requiresPress && binding.key == key) {

                if (binding.action == InputAction::EXIT_APPLICATION) {
                    std::cout << "EXIT requested!" << std::endl;
                    return; 
                }

                auto it = m_actionCallbacks.find(binding.action);
                if (it != m_actionCallbacks.end() && it->second) {
                    try {
                        it->second();
                    }
                    catch (const std::exception& e) {
                        std::cerr << "Error in action callback: " << e.what() << std::endl;
                    }
                }
                break;
            }
        }
    }
    else if (action == GLFW_RELEASE) {
        m_pressedKeys.erase(key);
        m_justReleasedKeys.insert(key);
    }
}

void InputHandler::onMouseMove(double xpos, double ypos) {
    if (m_cameraEnabled && m_camera && m_stateManager && !m_stateManager->isPaused()) {
        m_camera->processMouseMovement(static_cast<float>(xpos), static_cast<float>(ypos));
    }
}

void InputHandler::processKeyAction(InputAction action) {
    switch (action) {
    case InputAction::EXIT_APPLICATION:
        std::cout << "Exit application requested" << std::endl;
        break;
    default:
        auto it = m_actionCallbacks.find(action);
        if (it != m_actionCallbacks.end() && it->second) {
            it->second();
        }
        break;
    }
}

void InputHandler::setCameraEnabled(bool enabled) {
    m_cameraEnabled = enabled;

    if (m_window) {
        if (enabled) {
            m_window->setCursorMode(GLFW_CURSOR_DISABLED);
            if (m_camera) {
                m_camera->resetMouseState();
            }
            std::cout << "Camera control enabled" << std::endl;
        }
        else {
            m_window->setCursorMode(GLFW_CURSOR_NORMAL);
            std::cout << "Camera control disabled" << std::endl;
        }
    }
}

void InputHandler::toggleCameraControl() {
    setCameraEnabled(!m_cameraEnabled);
}

void InputHandler::addKeyBinding(int key, InputAction action, bool requiresPress) {
    m_keyBindings.push_back({ key, action, requiresPress });
}

void InputHandler::removeKeyBinding(int key) {
    m_keyBindings.erase(
        std::remove_if(m_keyBindings.begin(), m_keyBindings.end(),
            [key](const KeyBinding& binding) { return binding.key == key; }),
        m_keyBindings.end());
}

void InputHandler::clearKeyBindings() {
    m_keyBindings.clear();
}

void InputHandler::setActionCallback(InputAction action, std::function<void()> callback) {
    m_actionCallbacks[action] = callback;
}

bool InputHandler::isKeyPressed(int key) const {
    return m_pressedKeys.find(key) != m_pressedKeys.end();
}

bool InputHandler::isKeyJustPressed(int key) const {
    return m_justPressedKeys.find(key) != m_justPressedKeys.end();
}

bool InputHandler::isKeyJustReleased(int key) const {
    return m_justReleasedKeys.find(key) != m_justReleasedKeys.end();
}

void InputHandler::printKeyBindings() const {
    std::cout << "=== KEY BINDINGS ===" << std::endl;
    std::cout << "WASD + Space/Shift - Move camera" << std::endl;
    std::cout << "C - Toggle camera control" << std::endl;
    std::cout << "F - Toggle fullscreen" << std::endl;
    std::cout << "E - Trigger explosion" << std::endl;
    std::cout << "R - Reset to intact building" << std::endl;
    std::cout << "I - Debug info" << std::endl;
    std::cout << "ESC - Exit" << std::endl;
    std::cout << "====================" << std::endl;
}