#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <vector>
#include <chrono>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include <sstream>

enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARNING = 3,
    ERROR = 4,
    FATAL = 5
};

struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    LogLevel level;
    std::string category;
    std::string message;
    std::string file;
    int line;
    std::string function;
    std::thread::id threadId;
};

class LogOutput {
public:
    virtual ~LogOutput() = default;
    virtual void write(const LogEntry& entry) = 0;
    virtual void flush() = 0;
};

class ConsoleOutput : public LogOutput {
private:
    LogLevel m_minLevel;

public:
    explicit ConsoleOutput(bool useColors = false, LogLevel minLevel = LogLevel::INFO);
    void write(const LogEntry& entry) override;
    void flush() override;
    void setMinLevel(LogLevel level) { m_minLevel = level; }
};

class FileOutput : public LogOutput {
private:
    std::ofstream m_file;
    std::string m_filename;
    LogLevel m_minLevel;
    bool m_autoFlush;
    size_t m_maxFileSize;
    int m_maxFiles;

    void rotateLogFile();

public:
    explicit FileOutput(const std::string& filename,
        LogLevel minLevel = LogLevel::TRACE,
        bool autoFlush = true,
        size_t maxFileSize = 10 * 1024 * 1024, 
        int maxFiles = 5);
    ~FileOutput();

    void write(const LogEntry& entry) override;
    void flush() override;
    bool isOpen() const { return m_file.is_open(); }
};

class CallbackOutput : public LogOutput {
private:
    std::function<void(const LogEntry&)> m_callback;
    LogLevel m_minLevel;

public:
    explicit CallbackOutput(std::function<void(const LogEntry&)> callback,
        LogLevel minLevel = LogLevel::INFO);
    void write(const LogEntry& entry) override;
    void flush() override;
};

class Logger {
private:
    static std::unique_ptr<Logger> s_instance;
    static std::mutex s_instanceMutex;

    std::vector<std::unique_ptr<LogOutput>> m_outputs;
    std::queue<LogEntry> m_logQueue;
    std::mutex m_queueMutex;
    std::thread m_logThread;
    bool m_running;
    LogLevel m_globalMinLevel;

    std::atomic<size_t> m_totalLogMessages{ 0 };
    std::atomic<size_t> m_droppedMessages{ 0 };
    size_t m_maxQueueSize = 10000;

    std::string formatEntry(const LogEntry& entry) const;
    std::string levelToString(LogLevel level) const;

    void logWorker();

    Logger();

public:
    ~Logger();

    static Logger& getInstance();
    static void shutdown();

    void addOutput(std::unique_ptr<LogOutput> output);
    void setGlobalMinLevel(LogLevel level) { m_globalMinLevel = level; }
    void setMaxQueueSize(size_t maxSize) { m_maxQueueSize = maxSize; }

    void log(LogLevel level, const std::string& category, const std::string& message,
        const std::string& file = "", int line = 0, const std::string& function = "");

    template<typename... Args>
    void logf(LogLevel level, const std::string& category, const std::string& format, Args&&... args) {
        std::ostringstream oss;
        formatMessage(oss, format, std::forward<Args>(args)...);
        log(level, category, oss.str());
    }

    void trace(const std::string& category, const std::string& message,
        const std::string& file = "", int line = 0, const std::string& function = "");
    void debug(const std::string& category, const std::string& message,
        const std::string& file = "", int line = 0, const std::string& function = "");
    void info(const std::string& category, const std::string& message,
        const std::string& file = "", int line = 0, const std::string& function = "");
    void warning(const std::string& category, const std::string& message,
        const std::string& file = "", int line = 0, const std::string& function = "");
    void error(const std::string& category, const std::string& message,
        const std::string& file = "", int line = 0, const std::string& function = "");
    void fatal(const std::string& category, const std::string& message,
        const std::string& file = "", int line = 0, const std::string& function = "");

    template<typename... Args>
    void tracef(const std::string& category, const std::string& format, Args&&... args) {
        logf(LogLevel::TRACE, category, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debugf(const std::string& category, const std::string& format, Args&&... args) {
        logf(LogLevel::DEBUG, category, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void infof(const std::string& category, const std::string& format, Args&&... args) {
        logf(LogLevel::INFO, category, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warningf(const std::string& category, const std::string& format, Args&&... args) {
        logf(LogLevel::WARNING, category, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void errorf(const std::string& category, const std::string& format, Args&&... args) {
        logf(LogLevel::ERROR, category, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void fatalf(const std::string& category, const std::string& format, Args&&... args) {
        logf(LogLevel::FATAL, category, format, std::forward<Args>(args)...);
    }

    size_t getTotalLogMessages() const { return m_totalLogMessages.load(); }
    size_t getDroppedMessages() const { return m_droppedMessages.load(); }
    size_t getQueueSize() const;

    void flush();

private:
    template<typename T>
    void formatMessage(std::ostringstream& oss, const std::string& format, T&& value) {
        size_t pos = format.find("{}");
        if (pos != std::string::npos) {
            oss << format.substr(0, pos) << value << format.substr(pos + 2);
        }
        else {
            oss << format;
        }
    }

    template<typename T, typename... Args>
    void formatMessage(std::ostringstream& oss, const std::string& format, T&& value, Args&&... args) {
        size_t pos = format.find("{}");
        if (pos != std::string::npos) {
            oss << format.substr(0, pos) << value;
            formatMessage(oss, format.substr(pos + 2), std::forward<Args>(args)...);
        }
        else {
            oss << format;
        }
    }
};

#define LOG_TRACE(category, message) \
    Logger::getInstance().trace(category, message, __FILE__, __LINE__, __func__)

#define LOG_DEBUG(category, message) \
    Logger::getInstance().debug(category, message, __FILE__, __LINE__, __func__)

#define LOG_INFO(category, message) \
    Logger::getInstance().info(category, message, __FILE__, __LINE__, __func__)

#define LOG_WARNING(category, message) \
    Logger::getInstance().warning(category, message, __FILE__, __LINE__, __func__)

#define LOG_ERROR(category, message) \
    Logger::getInstance().error(category, message, __FILE__, __LINE__, __func__)

#define LOG_FATAL(category, message) \
    Logger::getInstance().fatal(category, message, __FILE__, __LINE__, __func__)

#define LOG_TRACEF(category, format, ...) \
    Logger::getInstance().tracef(category, format, __VA_ARGS__)

#define LOG_DEBUGF(category, format, ...) \
    Logger::getInstance().debugf(category, format, __VA_ARGS__)

#define LOG_INFOF(category, format, ...) \
    Logger::getInstance().infof(category, format, __VA_ARGS__)

#define LOG_WARNINGF(category, format, ...) \
    Logger::getInstance().warningf(category, format, __VA_ARGS__)

#define LOG_ERRORF(category, format, ...) \
    Logger::getInstance().errorf(category, format, __VA_ARGS__)

#define LOG_FATALF(category, format, ...) \
    Logger::getInstance().fatalf(category, format, __VA_ARGS__)

class LogTimer {
private:
    std::string m_category;
    std::string m_operation;
    std::chrono::steady_clock::time_point m_start;

public:
    LogTimer(const std::string& category, const std::string& operation);
    ~LogTimer();

    void finish();
    float getElapsedMs() const;
};

#define LOG_TIMER(category, operation) LogTimer timer(category, operation)