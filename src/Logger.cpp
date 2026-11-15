#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#undef ERROR  
#else
#include <cstring>
#endif

#include "Logger.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <thread>
#include <atomic>

std::unique_ptr<Logger> Logger::s_instance = nullptr;
std::mutex Logger::s_instanceMutex;


ConsoleOutput::ConsoleOutput(bool useColors, LogLevel minLevel)
    : m_minLevel(minLevel) {
    // Ignora useColors - non usiamo colori
}

void ConsoleOutput::write(const LogEntry& entry) {
    if (entry.level < m_minLevel) return;

    std::string levelStr;
    switch (entry.level) {
    case LogLevel::TRACE:   levelStr = "TRACE"; break;
    case LogLevel::DEBUG:   levelStr = "DEBUG"; break;
    case LogLevel::INFO:    levelStr = "INFO"; break;
    case LogLevel::WARNING: levelStr = "WARNING"; break;
    case LogLevel::ERROR:   levelStr = "ERROR"; break;
    case LogLevel::FATAL:   levelStr = "FATAL"; break;
    }

    auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
    char timeBuffer[16];
    struct tm timeInfo;
#ifdef _WIN32
    localtime_s(&timeInfo, &time_t);
#else
    localtime_r(&time_t, &timeInfo);
#endif
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &timeInfo);

    std::cout << "[" << timeBuffer << "] "
        << "[" << levelStr << "] "
        << "[" << entry.category << "] "
        << entry.message << std::endl;
}

void ConsoleOutput::flush() {
    std::cout.flush();
}

FileOutput::FileOutput(const std::string& filename, LogLevel minLevel, bool autoFlush,
    size_t maxFileSize, int maxFiles)
    : m_filename(filename), m_minLevel(minLevel), m_autoFlush(autoFlush),
    m_maxFileSize(maxFileSize), m_maxFiles(maxFiles) {

    m_file.open(filename, std::ios::app);
    if (!m_file.is_open()) {
        std::cerr << "Failed to open log file: " << filename << std::endl;
    }
}

FileOutput::~FileOutput() {
    if (m_file.is_open()) {
        m_file.close();
    }
}

void FileOutput::write(const LogEntry& entry) {
    if (!m_file.is_open() || entry.level < m_minLevel) return;

    if (m_file.tellp() > static_cast<std::streampos>(m_maxFileSize)) {
        rotateLogFile();
    }

    auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
    char timeBuffer[32];
    struct tm timeInfo;
#ifdef _WIN32
    localtime_s(&timeInfo, &time_t);
#else
    localtime_r(&time_t, &timeInfo);
#endif
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &timeInfo);

    std::string levelStr;
    switch (entry.level) {
    case LogLevel::TRACE: levelStr = "TRACE"; break;
    case LogLevel::DEBUG: levelStr = "DEBUG"; break;
    case LogLevel::INFO: levelStr = "INFO"; break;
    case LogLevel::WARNING: levelStr = "WARNING"; break;
    case LogLevel::ERROR: levelStr = "ERROR"; break;
    case LogLevel::FATAL: levelStr = "FATAL"; break;
    }

    m_file << "[" << timeBuffer << "] "
        << "[" << levelStr << "] "
        << "[" << entry.category << "] "
        << entry.message;

    if (!entry.file.empty()) {
        m_file << " (" << entry.file << ":" << entry.line << ")";
    }

    m_file << std::endl;

    if (m_autoFlush) {
        m_file.flush();
    }
}

void FileOutput::flush() {
    if (m_file.is_open()) {
        m_file.flush();
    }
}

void FileOutput::rotateLogFile() {
    if (!m_file.is_open()) return;

    m_file.close();

    for (int i = m_maxFiles - 1; i > 0; --i) {
        std::string oldFile = m_filename + "." + std::to_string(i);
        std::string newFile = m_filename + "." + std::to_string(i + 1);
        std::rename(oldFile.c_str(), newFile.c_str());
    }

    std::string backupFile = m_filename + ".1";
    std::rename(m_filename.c_str(), backupFile.c_str());

    m_file.open(m_filename, std::ios::out | std::ios::trunc);
}

CallbackOutput::CallbackOutput(std::function<void(const LogEntry&)> callback, LogLevel minLevel)
    : m_callback(callback), m_minLevel(minLevel) {
}

void CallbackOutput::write(const LogEntry& entry) {
    if (entry.level >= m_minLevel && m_callback) {
        m_callback(entry);
    }
}

void CallbackOutput::flush() {
    // No action needed for callback output
}


Logger::Logger() : m_running(true), m_globalMinLevel(LogLevel::TRACE) {
    // Start worker thread
    m_logThread = std::thread(&Logger::logWorker, this);
}

Logger::~Logger() {
    shutdown();
}

Logger& Logger::getInstance() {
    std::lock_guard<std::mutex> lock(s_instanceMutex);
    if (!s_instance) {
        s_instance = std::unique_ptr<Logger>(new Logger());
    }
    return *s_instance;
}

void Logger::shutdown() {
    if (s_instance) {
        s_instance->m_running = false;

        if (s_instance->m_logThread.joinable()) {
            s_instance->m_logThread.join();
        }

        // Flush all outputs
        for (auto& output : s_instance->m_outputs) {
            output->flush();
        }

        s_instance.reset();
    }
}

void Logger::addOutput(std::unique_ptr<LogOutput> output) {
    if (output) {
        m_outputs.push_back(std::move(output));
    }
}

void Logger::log(LogLevel level, const std::string& category, const std::string& message,
    const std::string& file, int line, const std::string& function) {

    if (level < m_globalMinLevel) return;

    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = level;
    entry.category = category;
    entry.message = message;
    entry.file = file;
    entry.line = line;
    entry.function = function;
    entry.threadId = std::this_thread::get_id();

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (m_logQueue.size() < m_maxQueueSize) {
            m_logQueue.push(entry);
            m_totalLogMessages++;
        }
        else {
            m_droppedMessages++;
        }
    }
}

void Logger::trace(const std::string& category, const std::string& message,
    const std::string& file, int line, const std::string& function) {
    log(LogLevel::TRACE, category, message, file, line, function);
}

void Logger::debug(const std::string& category, const std::string& message,
    const std::string& file, int line, const std::string& function) {
    log(LogLevel::DEBUG, category, message, file, line, function);
}

void Logger::info(const std::string& category, const std::string& message,
    const std::string& file, int line, const std::string& function) {
    log(LogLevel::INFO, category, message, file, line, function);
}

void Logger::warning(const std::string& category, const std::string& message,
    const std::string& file, int line, const std::string& function) {
    log(LogLevel::WARNING, category, message, file, line, function);
}

void Logger::error(const std::string& category, const std::string& message,
    const std::string& file, int line, const std::string& function) {
    log(LogLevel::ERROR, category, message, file, line, function);
}

void Logger::fatal(const std::string& category, const std::string& message,
    const std::string& file, int line, const std::string& function) {
    log(LogLevel::FATAL, category, message, file, line, function);
}

void Logger::flush() {
    for (auto& output : m_outputs) {
        output->flush();
    }
}

size_t Logger::getQueueSize() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_queueMutex));
    return m_logQueue.size();
}

void Logger::logWorker() {
    while (m_running) {
        LogEntry entry;
        bool hasEntry = false;

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (!m_logQueue.empty()) {
                entry = m_logQueue.front();
                m_logQueue.pop();
                hasEntry = true;
            }
        }

        if (hasEntry) {
            for (auto& output : m_outputs) {
                output->write(entry);
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    while (true) {
        LogEntry entry;
        bool hasEntry = false;

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (!m_logQueue.empty()) {
                entry = m_logQueue.front();
                m_logQueue.pop();
                hasEntry = true;
            }
        }

        if (!hasEntry) break;

        for (auto& output : m_outputs) {
            output->write(entry);
        }
    }
}

std::string Logger::formatEntry(const LogEntry& entry) const {
    auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);

    char timeBuffer[16];
    struct tm timeInfo;
#ifdef _WIN32
    localtime_s(&timeInfo, &time_t);
#else
    localtime_r(&time_t, &timeInfo);
#endif
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &timeInfo);

    std::ostringstream oss;
    oss << "[" << timeBuffer << "] "
        << "[" << levelToString(entry.level) << "] "
        << "[" << entry.category << "] "
        << entry.message;

    return oss.str();
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
    case LogLevel::TRACE: return "TRACE";
    case LogLevel::DEBUG: return "DEBUG";
    case LogLevel::INFO: return "INFO";
    case LogLevel::WARNING: return "WARNING";
    case LogLevel::ERROR: return "ERROR";
    case LogLevel::FATAL: return "FATAL";
    default: return "UNKNOWN";
    }
}

LogTimer::LogTimer(const std::string& category, const std::string& operation)
    : m_category(category), m_operation(operation),
    m_start(std::chrono::steady_clock::now()) {
}

LogTimer::~LogTimer() {
    if (m_start.time_since_epoch().count() > 0) {
        finish();
    }
}

void LogTimer::finish() {
    float elapsedMs = getElapsedMs();
    Logger::getInstance().infof(m_category, "Operation '{}' completed in {:.2f}ms", m_operation, elapsedMs);
    m_start = std::chrono::steady_clock::time_point{};
}

float LogTimer::getElapsedMs() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<float, std::milli>(now - m_start).count();
}