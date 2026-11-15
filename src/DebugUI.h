#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <string>
#include <utility>
#include <chrono>

class PerformanceMonitor;
class PhysicsSystem;
class Logger;
struct GLFWwindow;

class UIPanel {
public:
    virtual ~UIPanel() = default;
    virtual void render() = 0;
    virtual const char* getName() const = 0;
    virtual bool isVisible() const { return m_visible; }
    virtual void setVisible(bool visible) { m_visible = visible; }

protected:
    bool m_visible = true;
};

class PerformancePanel : public UIPanel {
private:
    PerformanceMonitor* m_performanceMonitor;
    bool m_showGraphs = true;
    bool m_showDetailedMetrics = false;
    int m_selectedMetric = 0;

public:
    explicit PerformancePanel(PerformanceMonitor* monitor);
    void render() override;
    const char* getName() const override { return "Performance"; }
};

class PhysicsPanel : public UIPanel {
private:
    PhysicsSystem* m_physicsSystem;
    bool m_showForceVectors = false;
    bool m_showCollisionShapes = false;
    bool m_showVelocityVectors = false;
    float m_forceScale = 1.0f;

public:
    explicit PhysicsPanel(PhysicsSystem* physics);
    void render() override;
    const char* getName() const override { return "Physics"; }
};

class LoggerPanel : public UIPanel {
private:
    Logger* m_logger;
    std::vector<std::string> m_logBuffer;
    int m_maxLogLines = 1000;
    bool m_autoScroll = true;
    bool m_showTimestamps = true;
    bool m_showCategories = true;
    int m_minLogLevel = 2;
    std::string m_filterText;

    void addLogEntry(const std::string& entry);

public:
    explicit LoggerPanel(Logger* logger);
    void render() override;
    const char* getName() const override { return "Logs"; }

    void setMaxLogLines(int maxLines) { m_maxLogLines = maxLines; }
};

class SystemInfoPanel : public UIPanel {
private:
    PerformanceMonitor* m_performanceMonitor;
    bool m_showHardwareInfo = true;
    bool m_showOpenGLInfo = true;

    void updateSystemInfo();

public:
    explicit SystemInfoPanel(PerformanceMonitor* monitor);
    void render() override;
    const char* getName() const override { return "System Info"; }
};

class ScenePanel : public UIPanel {
private:
    struct SceneStats {
        int totalObjects = 0;
        int visibleObjects = 0;
        int drawCalls = 0;
        int triangles = 0;
        int vertices = 0;
    } m_sceneStats;

    bool m_showBoundingBoxes = false;
    bool m_showWireframe = false;
    bool m_showNormals = false;
    float m_wireframeWidth = 1.0f;

public:
    ScenePanel();
    void render() override;
    const char* getName() const override { return "Scene"; }

    void updateStats(const SceneStats& stats) { m_sceneStats = stats; }
};

class ControlsPanel : public UIPanel {
private:
    std::vector<std::pair<std::string, std::string>> m_controls;

public:
    ControlsPanel();
    void render() override;
    const char* getName() const override { return "Controls"; }

    void addControl(const std::string& key, const std::string& description);
};

class DebugUI {
private:
    GLFWwindow* m_window;
    bool m_initialized = false;
    bool m_visible = true;

    std::vector<std::unique_ptr<UIPanel>> m_panels;

    bool m_showMainMenuBar = true;
    bool m_showDemoWindow = false;
    bool m_showStyleEditor = false;
    bool m_showMetricsWindow = false;

    enum class Theme {
        DARK,
        LIGHT,
        CLASSIC,
        CUSTOM
    } m_currentTheme = Theme::DARK;

    float m_uiScale = 1.0f;
    float m_alpha = 0.95f;

    float m_uiFrameTime = 0.0f;

    void setupImGuiStyle();
    void applyTheme(Theme theme);
    void renderMainMenuBar();
    void renderPanels();
    void renderThemeSelector();

public:
    DebugUI();
    ~DebugUI();

    bool initialize(GLFWwindow* window);
    void shutdown();
    void beginFrame();
    void endFrame();
    void render();

    void addPanel(std::unique_ptr<UIPanel> panel);
    template<typename T>
    T* getPanel() {
        for (auto& panel : m_panels) {
            if (T* p = dynamic_cast<T*>(panel.get())) {
                return p;
            }
        }
        return nullptr;
    }

    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    void toggleVisibility() { m_visible = !m_visible; }

    void setTheme(Theme theme);
    void setUIScale(float scale);
    void setAlpha(float alpha);

    float getUIFrameTime() const { return m_uiFrameTime; }

    static void HelpMarker(const char* desc);
    static void PlotLines(const char* label, const std::vector<float>& values,
        float scale_min = 0.0f, float scale_max = 0.0f,
        const char* overlay_text = nullptr);
    static bool ColoredButton(const char* label, const float* color, const float* hovered_color = nullptr);

    std::unique_ptr<PerformancePanel> createPerformancePanel(PerformanceMonitor* monitor);
    std::unique_ptr<PhysicsPanel> createPhysicsPanel(PhysicsSystem* physics);
    std::unique_ptr<LoggerPanel> createLoggerPanel(Logger* logger);
    std::unique_ptr<SystemInfoPanel> createSystemInfoPanel(PerformanceMonitor* monitor);
    std::unique_ptr<ScenePanel> createScenePanel();
    std::unique_ptr<ControlsPanel> createControlsPanel();
};