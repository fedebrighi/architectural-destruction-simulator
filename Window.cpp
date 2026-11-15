#include "Window.h"
#include <iostream>

Window::Window(int width, int height, const std::string& title)
    : m_window(nullptr), m_width(width), m_height(height), m_title(title),
    m_isFullscreen(false), m_windowedWidth(width), m_windowedHeight(height),
    m_windowedPosX(100), m_windowedPosY(100) {
}

Window::~Window() {
    cleanup();
}

bool Window::initialize() {
    glfwSetErrorCallback(errorCallback);

    // Inizializzo GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Configuro GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    // Creo la window
    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);

    glfwSetWindowUserPointer(m_window, this);

    // Setto i callbacks
    glfwSetKeyCallback(m_window, keyCallbackWrapper);
    glfwSetCursorPosCallback(m_window, mouseCallbackWrapper);
    glfwSetFramebufferSizeCallback(m_window, framebufferCallbackWrapper);

    // Inizializzo GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    // Setto la viewport
    glViewport(0, 0, m_width, m_height);

    glfwSwapInterval(0);

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GPU: " << glGetString(GL_RENDERER) << std::endl;

    return true;
}

void Window::cleanup() {
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window);
}

void Window::swapBuffers() {
    glfwSwapBuffers(m_window);
}

void Window::pollEvents() {
    glfwPollEvents();
}

bool Window::isKeyPressed(int key) const {
    return glfwGetKey(m_window, key) == GLFW_PRESS;
}

void Window::setCursorMode(int mode) {
    glfwSetInputMode(m_window, GLFW_CURSOR, mode);
}

void Window::keyCallbackWrapper(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win && win->m_keyCallback) {
        win->m_keyCallback(key, scancode, action, mods);
    }
}

void Window::mouseCallbackWrapper(GLFWwindow* window, double xpos, double ypos) {
    Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win && win->m_mouseCallback) {
        win->m_mouseCallback(xpos, ypos);
    }
}

void Window::framebufferCallbackWrapper(GLFWwindow* window, int width, int height) {
    Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win) {
        win->m_width = width;
        win->m_height = height;
        glViewport(0, 0, width, height);

        if (win->m_framebufferCallback) {
            win->m_framebufferCallback(width, height);
        }
    }
}

void Window::errorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

void Window::toggleFullscreen() {
    if (m_isFullscreen) {
        glfwSetWindowMonitor(m_window, nullptr, m_windowedPosX, m_windowedPosY,
            m_windowedWidth, m_windowedHeight, GLFW_DONT_CARE);
        m_width = m_windowedWidth;
        m_height = m_windowedHeight;
        m_isFullscreen = false;
        std::cout << "Switched to windowed mode (" << m_width << "x" << m_height << ")" << std::endl;
    }
    else {
        glfwGetWindowPos(m_window, &m_windowedPosX, &m_windowedPosY);
        glfwGetWindowSize(m_window, &m_windowedWidth, &m_windowedHeight);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        m_width = mode->width;
        m_height = mode->height;
        m_isFullscreen = true;
        std::cout << "Switched to fullscreen (" << m_width << "x" << m_height << ")" << std::endl;
    }

    glViewport(0, 0, m_width, m_height);
}