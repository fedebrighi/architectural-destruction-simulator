#pragma once

#include <functional>
#include <unordered_map>
#include <unordered_set>

class Window;
class Camera;
class StateManager;

enum class InputAction {
    MOVE_FORWARD,
    MOVE_BACKWARD,
    MOVE_LEFT,
    MOVE_RIGHT,
    MOVE_UP,
    MOVE_DOWN,
    TOGGLE_CAMERA,
    TOGGLE_FULLSCREEN,
    TRIGGER_EXPLOSION,
    RESET_EXPLOSION,
    PAUSE_SIMULATION,
    EXIT_APPLICATION,
    DEBUG_INFO
};

struct KeyBinding {
    int key;
    InputAction action;
    bool requiresPress;
};

class InputHandler {
private:
    Window* m_window;
    Camera* m_camera;
    StateManager* m_stateManager;

    std::unordered_set<int> m_pressedKeys;
    std::unordered_set<int> m_justPressedKeys;
    std::unordered_set<int> m_justReleasedKeys;

    std::vector<KeyBinding> m_keyBindings;

    bool m_cameraEnabled;

    std::unordered_map<InputAction, std::function<void()>> m_actionCallbacks;

    void setupDefaultKeyBindings();
    void processKeyAction(InputAction action);

public:
    InputHandler();
    ~InputHandler();

    void initialize(Window* window, Camera* camera, StateManager* stateManager);
    void setupCallbacks();

    void update(float deltaTime);
    void processInput(float deltaTime);

    void onKeyPressed(int key, int scancode, int action, int mods);
    void onMouseMove(double xpos, double ypos);

    bool isCameraEnabled() const { return m_cameraEnabled; }
    void setCameraEnabled(bool enabled);
    void toggleCameraControl();

    void addKeyBinding(int key, InputAction action, bool requiresPress = true);
    void removeKeyBinding(int key);
    void clearKeyBindings();

    void setActionCallback(InputAction action, std::function<void()> callback);

    bool isKeyPressed(int key) const;
    bool isKeyJustPressed(int key) const;
    bool isKeyJustReleased(int key) const;

    void printKeyBindings() const;
};