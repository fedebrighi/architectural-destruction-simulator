#include "Camera.h"
#include <algorithm>
#include <cmath>

template<typename T>

constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
    return (v < lo) ? lo : (hi < v) ? hi : v;
}

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : m_position(position), m_worldUp(up), m_yaw(yaw), m_pitch(pitch),
    m_movementSpeed(SPEED), m_mouseSensitivity(SENSITIVITY), m_zoom(ZOOM),
    m_firstMouse(true), m_lastX(0.0f), m_lastY(0.0f) {
    updateCameraVectors();
}

Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
    : m_position(glm::vec3(posX, posY, posZ)), m_worldUp(glm::vec3(upX, upY, upZ)),
    m_yaw(yaw), m_pitch(pitch), m_movementSpeed(SPEED), m_mouseSensitivity(SENSITIVITY),
    m_zoom(ZOOM), m_firstMouse(true), m_lastX(0.0f), m_lastY(0.0f) {
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio, float nearPlane, float farPlane) const {
    return glm::perspective(glm::radians(m_zoom), aspectRatio, nearPlane, farPlane);
}

void Camera::processKeyboard(Camera_Movement direction, float deltaTime) {
    float velocity = m_movementSpeed * deltaTime;

    switch (direction) {
    case FORWARD:
        m_position += m_front * velocity;
        break;
    case BACKWARD:
        m_position -= m_front * velocity;
        break;
    case LEFT:
        m_position -= m_right * velocity;
        break;
    case RIGHT:
        m_position += m_right * velocity;
        break;
    case UP:
        m_position += m_worldUp * velocity;
        break;
    case DOWN:
        m_position -= m_worldUp * velocity;
        break;
    }
}

void Camera::processMouseMovement(float xpos, float ypos, bool constrainPitch) {
    if (m_firstMouse) {
        m_lastX = xpos;
        m_lastY = ypos;
        m_firstMouse = false;
    }

    float xoffset = xpos - m_lastX;
    float yoffset = m_lastY - ypos;

    m_lastX = xpos;
    m_lastY = ypos;

    xoffset *= m_mouseSensitivity;
    yoffset *= m_mouseSensitivity;

    m_yaw += xoffset;
    m_pitch += yoffset;

    if (constrainPitch) {
        m_pitch = clamp(m_pitch, -89.0f, 89.0f);
    }

    updateCameraVectors();
}

void Camera::processMouseScroll(float yoffset) {
    m_zoom -= yoffset;
    m_zoom = clamp(m_zoom, 1.0f, 90.0f);
}

void Camera::setPosition(const glm::vec3& position) {
    m_position = position;
}

void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front = glm::normalize(front);

    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}