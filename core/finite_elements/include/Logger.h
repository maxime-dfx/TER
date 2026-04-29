#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>

enum class LogLevel { DEBUG, INFO, WARNING, ERROR, PROGRESS };

class Logger {
private:
    std::mutex logMutex; // Empêche les threads OpenMP de se marcher dessus
    LogLevel minLevel;

    Logger() : minLevel(LogLevel::INFO) {}
    
    std::string levelToString(LogLevel level) {
        switch(level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARN";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::PROGRESS: return "PROG";
            default: return "UNKNOWN";
        }
    }

public:
    // Singleton
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    // Désactiver la copie
    Logger(Logger const&) = delete;
    void operator=(Logger const&) = delete;

    void setLevel(LogLevel level) { minLevel = level; }

    void log(LogLevel level, const std::string& context, const std::string& message) {
        if (level < minLevel) return;

        // Mutex pour thread-safety avec OpenMP
        std::lock_guard<std::mutex> lock(logMutex);
        
        // Format industriel : [LEVEL] [CONTEXT] Message
        std::cout << "[" << levelToString(level) << "] [" << context << "] " << message << std::endl;
    }
};

// Macros pour rendre le code ultra-propre (comme dans les frameworks pros)
#define LOG_DEBUG(msg) Logger::getInstance().log(LogLevel::DEBUG, __func__, msg)
#define LOG_INFO(msg)  Logger::getInstance().log(LogLevel::INFO, __func__, msg)
#define LOG_WARN(msg)  Logger::getInstance().log(LogLevel::WARNING, __func__, msg)
#define LOG_ERROR(msg) Logger::getInstance().log(LogLevel::ERROR, __func__, msg)
#define LOG_PROG(msg)  Logger::getInstance().log(LogLevel::PROGRESS, __func__, msg)

#endif