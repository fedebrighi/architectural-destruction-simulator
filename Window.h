#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <functional>

class Window {
private:
    GLFWwindow* m_window;
    int m_width;
    int m_height;
    std::string m_title;

    std::function<void(int, int, int, int)> m_keyCallback;
    std::function<void(double, double)> m_mouseCallback;
    std::function<void(int, int)> m_framebufferCallback;

    static void keyCallbackWrapper(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseCallbackWrapper(GLFWwindow* window, double xpos, double ypos);
    static void framebufferCallbackWrapper(GLFWwindow* window, int width, int height);
    static void errorCallback(int error, const char* description);

public:
    Window(int width, int height, const std::string& title);
    ~Window();

    bool initialize();
    void cleanup();

    bool shouldClose() const;
    void swapBuffers();
    void pollEvents();

    GLFWwindow* getGLFWWindow() const { return m_window; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    float getAspectRatio() const { return static_cast<float>(m_width) / static_cast<float>(m_height); }

    void setKeyCallback(std::function<void(int, int, int, int)> callback) { m_keyCallback = callback; }
    void setMouseCallback(std::function<void(double, double)> callback) { m_mouseCallback = callback; }
    void setFramebufferCallback(std::function<void(int, int)> callback) { m_framebufferCallback = callback; }

    bool isKeyPressed(int key) const;
    void setCursorMode(int mode);

    void toggleFullscreen();
    bool isFullscreen() const { return m_isFullscreen; }

private:
    bool m_isFullscreen;
    int m_windowedWidth, m_windowedHeight;
    int m_windowedPosX, m_windowedPosY;
};