#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Valori di Default della Camera
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera {
private:
    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;

    float m_yaw;
    float m_pitch;

    float m_movementSpeed;
    float m_mouseSensitivity;
    float m_zoom;

    bool m_firstMouse;
    float m_lastX;
    float m_lastY;

    void updateCameraVectors();

public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
        float yaw = YAW,
        float pitch = PITCH);

    Camera(float posX, float posY, float posZ,
        float upX, float upY, float upZ,
        float yaw, float pitch);

    glm::mat4 getViewMatrix() const;

    glm::mat4 getProjectionMatrix(float aspectRatio, float nearPlane = 0.1f, float farPlane = 100.0f) const;

    void processKeyboard(Camera_Movement direction, float deltaTime);

    void processMouseMovement(float xpos, float ypos, bool constrainPitch = true);

    void processMouseScroll(float yoffset);

    glm::vec3 getPosition() const { return m_position; }
    glm::vec3 getFront() const { return m_front; }
    glm::vec3 getUp() const { return m_up; }
    glm::vec3 getRight() const { return m_right; }
    float getZoom() const { return m_zoom; }
    float getYaw() const { return m_yaw; }
    float getPitch() const { return m_pitch; }

    void setPosition(const glm::vec3& position);
    void setSpeed(float speed) { m_movementSpeed = speed; }
    void setSensitivity(float sensitivity) { m_mouseSensitivity = sensitivity; }

    void resetMouseState() { m_firstMouse = true; }
};