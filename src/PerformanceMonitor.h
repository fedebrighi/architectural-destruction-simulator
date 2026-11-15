#pragma once

#include <chrono>
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>

#ifdef _WIN32
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGIAdapter;
struct IWbemLocator;
struct IWbemServices;
typedef struct _DXGI_ADAPTER_DESC DXGI_ADAPTER_DESC;
#endif

struct PerformanceMetrics {
    float currentFPS = 0.0f;
    float averageFPS = 0.0f;
    float minFPS = 0.0f;
    float maxFPS = 0.0f;
    float frameTime = 0.0f;           // ms per frame
    float deltaTime = 0.0f;           

    // Uso di Memoria
    float cpuMemoryUsageMB = 0.0f;    
    float gpuMemoryUsageMB = 0.0f;    
    float cpuMemoryPercent = 0.0f;    
    float gpuMemoryPercent = 0.0f;    

    // GPU PERFORMANCE
    float gpuUsagePercent = 0.0f;     
    float gpuTemperature = 0.0f;      
    float gpuCoreClockMHz = 0.0f;     
    float gpuMemoryClockMHz = 0.0f;   
    float gpuPowerUsageWatts = 0.0f;  

    // CPU performance (secondario)
    float cpuUsagePercent = 0.0f;     
    int activeThreads = 0;            

    int activeFragments = 0;
    int sleepingFragments = 0;
    float physicsTime = 0.0f;         
    float renderTime = 0.0f;          
    float collisionTime = 0.0f;       
    float computeShaderTime = 0.0f;   

    float networkLatency = 0.0f;
    float diskIORate = 0.0f;

    // Info di Sistema
    std::string gpuName = "Unknown";
    std::string cpuName = "Unknown";
    int totalSystemRAM = 0;           
    int availableSystemRAM = 0;      
    float totalVRAM = 0.0f;          
    std::string physicsMode = "Idle";  
    bool physicsActive = false;
};

struct PerformanceThresholds {
    float minAcceptableFPS = 30.0f;
    float maxAcceptableFrameTime = 33.33f;  
    float maxAcceptableGPUUsage = 95.0f;    
    float maxAcceptableGPUTemp = 80.0f;     
    float maxAcceptableVRAMUsage = 85.0f;   
    float maxAcceptableCPUUsage = 80.0f;  
    float maxAcceptableMemoryUsage = 85.0f; 
    float maxAcceptablePhysicsTime = 10.0f; 
    float maxAcceptableRenderTime = 16.0f;  
    float maxAcceptableComputeTime = 5.0f; 
};

struct PerformanceAlert {
    enum Type {
        LOW_FPS,
        HIGH_FRAME_TIME,
        HIGH_GPU_USAGE,        
        HIGH_GPU_TEMPERATURE,  
        HIGH_VRAM_USAGE,       
        HIGH_CPU_USAGE,       
        HIGH_MEMORY_USAGE,
        HIGH_PHYSICS_TIME,
        HIGH_RENDER_TIME,
        HIGH_COMPUTE_TIME,   
        GPU_OVERHEATING
    };

    Type type;
    std::string message;
    float value;
    float threshold;
    std::chrono::steady_clock::time_point timestamp;
    bool resolved = false;
};

class PerformanceMonitor {
private:
    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_lastFrameTime;
    std::chrono::steady_clock::time_point m_lastUpdateTime;
    std::vector<std::string> m_physicsModeHistory;

    PerformanceMetrics m_currentMetrics;
    PerformanceThresholds m_thresholds;

    std::vector<float> m_fpsHistory;
    std::vector<float> m_frameTimeHistory;
    std::vector<float> m_gpuUsageHistory;        
    std::vector<float> m_gpuTempHistory;         
    std::vector<float> m_vramUsageHistory;         
    std::vector<float> m_cpuUsageHistory;        
    std::vector<float> m_memoryUsageHistory;
    std::vector<float> m_physicsTimeHistory;
    std::vector<float> m_renderTimeHistory;
    std::vector<float> m_computeTimeHistory;    

    static constexpr size_t MAX_HISTORY_SIZE = 300;

    std::vector<PerformanceAlert> m_activeAlerts;
    std::vector<PerformanceAlert> m_alertHistory;

    std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_customTimers;
    std::unordered_map<std::string, float> m_customTimings;

    float m_updateInterval = 0.1f;    

    std::function<void(const PerformanceAlert&)> m_alertCallback;
    std::function<void(const PerformanceMetrics&)> m_metricsCallback;

#ifdef _WIN32
    ID3D11Device* m_d3dDevice = nullptr;
    ID3D11DeviceContext* m_d3dContext = nullptr;
    IDXGIAdapter* m_dxgiAdapter = nullptr;
    DXGI_ADAPTER_DESC* m_adapterDesc = nullptr;

    IWbemLocator* m_wbemLocator = nullptr;
    IWbemServices* m_wbemServices = nullptr;
    bool m_wmiInitialized = false;
#endif

    struct GPUMetrics {
        float usage = 0.0f;
        float temperature = 0.0f;
        float memoryUsage = 0.0f;
        float memoryTotal = 0.0f;
        float coreClockMHz = 0.0f;
        float memoryClockMHz = 0.0f;
        float powerUsageWatts = 0.0f;
        std::string gpuName;
    } m_gpuMetrics;

    void updateFPSMetrics(float deltaTime);
    void updateSystemMetrics();
    void updateHistoricalData();
    void checkPerformanceAlerts();
    void addToHistory(std::vector<float>& history, float value);

    void updateMemoryUsage();

    bool initializeGPUMonitoring();
    bool initializeWMI();
    void updateGPUMetrics();
    void updateGPUUsage();
    void updateVRAMUsage();
    void updateAdvancedGPUMetrics();

    void updateCPUUsage();

public:
    PerformanceMonitor();
    ~PerformanceMonitor();

    bool initialize();
    void update(float deltaTime);
    void shutdown();

    void beginFrame();
    void endFrame();

    void startTimer(const std::string& name);
    void endTimer(const std::string& name);
    float getTimerResult(const std::string& name) const;

    const PerformanceMetrics& getCurrentMetrics() const { return m_currentMetrics; }
    const std::vector<float>& getFPSHistory() const { return m_fpsHistory; }
    const std::vector<float>& getFrameTimeHistory() const { return m_frameTimeHistory; }

    // GPU METRICS (PRIORITARI)
    const std::vector<float>& getGPUUsageHistory() const { return m_gpuUsageHistory; }
    const std::vector<float>& getGPUTempHistory() const { return m_gpuTempHistory; }
    const std::vector<float>& getVRAMUsageHistory() const { return m_vramUsageHistory; }
    const std::vector<float>& getComputeTimeHistory() const { return m_computeTimeHistory; }

    // CPU metrics (secondari)
    const std::vector<float>& getCPUUsageHistory() const { return m_cpuUsageHistory; }
    const std::vector<float>& getMemoryUsageHistory() const { return m_memoryUsageHistory; }
    const std::vector<float>& getPhysicsTimeHistory() const { return m_physicsTimeHistory; }
    const std::vector<float>& getRenderTimeHistory() const { return m_renderTimeHistory; }

    // Alerts
    const std::vector<PerformanceAlert>& getActiveAlerts() const { return m_activeAlerts; }
    const std::vector<PerformanceAlert>& getAlertHistory() const { return m_alertHistory; }
    void clearAlert(size_t alertIndex);
    void clearAllAlerts();

    void setThresholds(const PerformanceThresholds& thresholds) { m_thresholds = thresholds; }
    const PerformanceThresholds& getThresholds() const { return m_thresholds; }
    void setUpdateInterval(float interval) { m_updateInterval = interval; }

    void setAlertCallback(std::function<void(const PerformanceAlert&)> callback) { m_alertCallback = callback; }
    void setMetricsCallback(std::function<void(const PerformanceMetrics&)> callback) { m_metricsCallback = callback; }

    void setPhysicsMetrics(int activeFragments, int sleepingFragments, float physicsTime);
    void setRenderTime(float renderTime);
    void setCollisionTime(float collisionTime);
    void setComputeShaderTime(float computeTime);

    // Utilities
    std::string getSystemInfo() const;
    std::string getPerformanceSummary() const;
    void exportMetricsToCSV(const std::string& filename) const;

    // Debug
    void printCurrentMetrics() const;
    void printSystemInfo() const;

    void setPhysicsMode(const std::string& mode);

    // Reset/clear data
    void resetMetrics();
    void clearHistory();
};