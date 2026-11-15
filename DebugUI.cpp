#include "DebugUI.h"
#include "PerformanceMonitor.h"
#include "PhysicsSystem.h"
#include "Logger.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <iomanip>

PerformancePanel::PerformancePanel(PerformanceMonitor* monitor)
    : m_performanceMonitor(monitor) {
}

void PerformancePanel::render() {
    if (!m_visible || !m_performanceMonitor) return;

    static int renderCount = 0;
    renderCount++;

    if (renderCount % 300 == 0) { 
        const auto& metrics = m_performanceMonitor->getCurrentMetrics();
        std::cout << "=== PERFORMANCE ===" << std::endl;
        std::cout << "FPS: " << metrics.currentFPS << std::endl;
        std::cout << "Frame: " << metrics.frameTime << "ms" << std::endl;
        std::cout << "Physics: " << metrics.physicsTime << "ms" << std::endl;
        std::cout << "Render: " << metrics.renderTime << "ms" << std::endl;
        std::cout << "==================" << std::endl;
    }
}

PhysicsPanel::PhysicsPanel(PhysicsSystem* physics)
    : m_physicsSystem(physics) {
}

void PhysicsPanel::render() {
    if (!m_visible || !m_physicsSystem) return;

    static int renderCount = 0;
    renderCount++;

    if (renderCount % 180 == 0) { 
        std::cout << "=== PHYSICS ===" << std::endl;
        std::cout << "Init: " << (m_physicsSystem->isInitialized() ? "YES" : "NO") << std::endl;
        std::cout << "Active: " << (m_physicsSystem->isSimulationActive() ? "YES" : "NO") << std::endl;
        std::cout << "Fragments: " << m_physicsSystem->getFragmentCount() << std::endl;
        std::cout << "===============" << std::endl;
    }
}

LoggerPanel::LoggerPanel(Logger* logger)
    : m_logger(logger) {
}

void LoggerPanel::render() {
    if (!m_visible) return;

    static bool shown = false;
    if (!shown) {
        std::cout << "=== LOGGER PANEL ACTIVE ===" << std::endl;
        shown = true;
    }
}

void LoggerPanel::addLogEntry(const std::string& entry) {
    std::cout << "[LOG] " << entry << std::endl;
}

SystemInfoPanel::SystemInfoPanel(PerformanceMonitor* monitor)
    : m_performanceMonitor(monitor) {
    updateSystemInfo();
}

void SystemInfoPanel::render() {
    if (!m_visible) return;

    static bool shown = false;
    if (!shown) {
        std::cout << "=== SYSTEM INFO PANEL ACTIVE ===" << std::endl;
        shown = true;
    }
}

void SystemInfoPanel::updateSystemInfo() {
    // Simple update
}


ScenePanel::ScenePanel() {
}

void ScenePanel::render() {
    if (!m_visible) return;

    static bool shown = false;
    if (!shown) {
        std::cout << "=== SCENE PANEL ACTIVE ===" << std::endl;
        shown = true;
    }
}


ControlsPanel::ControlsPanel() {
}

void ControlsPanel::render() {
    if (!m_visible) return;

    static bool shown = false;
    if (!shown) {
        std::cout << "=== CONTROLS ===" << std::endl;
        for (size_t i = 0; i < m_controls.size(); ++i) {
            std::cout << m_controls[i].first << " : " << m_controls[i].second << std::endl;
        }
        std::cout << "================" << std::endl;
        shown = true;
    }
}

void ControlsPanel::addControl(const std::string& key, const std::string& description) {
    m_controls.push_back(std::make_pair(key, description));
}

DebugUI::DebugUI()
    : m_window(nullptr) {
}

DebugUI::~DebugUI() {
    shutdown();
}

bool DebugUI::initialize(GLFWwindow* window) {
    m_window = window;
    m_initialized = true;

    std::cout << "Debug UI initialized (console mode)" << std::endl;
    return true;
}

void DebugUI::shutdown() {
    if (m_initialized) {
        m_initialized = false;
        std::cout << "Debug UI shutdown" << std::endl;
    }
}

void DebugUI::beginFrame() {
    // Minimal implementation
}

void DebugUI::endFrame() {
    // Minimal implementation
}

void DebugUI::render() {
    if (!m_initialized || !m_visible) return;

    renderPanels();
}

void DebugUI::addPanel(std::unique_ptr<UIPanel> panel) {
    if (panel) {
        std::cout << "Added panel: " << panel->getName() << std::endl;
        m_panels.push_back(std::move(panel));
    }
}

void DebugUI::setTheme(Theme theme) {
    m_currentTheme = theme;
}

void DebugUI::setUIScale(float scale) {
    m_uiScale = scale;
}

void DebugUI::setAlpha(float alpha) {
    m_alpha = alpha;
}

void DebugUI::setupImGuiStyle() {
    // Minimal implementation
}

void DebugUI::applyTheme(Theme theme) {
    m_currentTheme = theme;
}

void DebugUI::renderMainMenuBar() {
    // Minimal implementation
}

void DebugUI::renderPanels() {
    for (auto& panel : m_panels) {
        if (panel && panel->isVisible()) {
            panel->render();
        }
    }
}

void DebugUI::renderThemeSelector() {
    // Minimal implementation
}

// Static utility methods
void DebugUI::HelpMarker(const char* desc) {
    std::cout << "[HELP] " << desc << std::endl;
}

void DebugUI::PlotLines(const char* label, const std::vector<float>& values,
    float scale_min, float scale_max, const char* overlay_text) {

    std::cout << "[PLOT] " << label << " (" << values.size() << " points)" << std::endl;
}

bool DebugUI::ColoredButton(const char* label, const float* color, const float* hovered_color) {
    std::cout << "[BUTTON] " << label << std::endl;
    return false;
}

// Panel creation helpers
std::unique_ptr<PerformancePanel> DebugUI::createPerformancePanel(PerformanceMonitor* monitor) {
    return std::make_unique<PerformancePanel>(monitor);
}

std::unique_ptr<PhysicsPanel> DebugUI::createPhysicsPanel(PhysicsSystem* physics) {
    return std::make_unique<PhysicsPanel>(physics);
}

std::unique_ptr<LoggerPanel> DebugUI::createLoggerPanel(Logger* logger) {
    return std::make_unique<LoggerPanel>(logger);
}

std::unique_ptr<SystemInfoPanel> DebugUI::createSystemInfoPanel(PerformanceMonitor* monitor) {
    return std::make_unique<SystemInfoPanel>(monitor);
}

std::unique_ptr<ScenePanel> DebugUI::createScenePanel() {
    return std::make_unique<ScenePanel>();
}

std::unique_ptr<ControlsPanel> DebugUI::createControlsPanel() {
    return std::make_unique<ControlsPanel>();
}