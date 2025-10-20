// Loki Logger Header
#ifndef LOKI_LOGGER_H
#define LOKI_LOGGER_H

#include <Arduino.h>
#include <vector>

#define MAX_LOG_BUFFER_SIZE 100

enum LogLevel {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_CRITICAL = 4
};

struct LogEntry {
    unsigned long timestamp;
    LogLevel level;
    String message;
    String component;

    LogEntry() : timestamp(0), level(LOG_LEVEL_INFO) {}
};

class LokiLogger {
public:
    LokiLogger();
    bool begin();
    void setLogLevel(LogLevel level);

    // Logging methods
    void log(LogLevel level, const String& message, const String& component = "");
    void debug(const String& message, const String& component = "");
    void info(const String& message, const String& component = "");
    void warning(const String& message, const String& component = "");
    void error(const String& message, const String& component = "");
    void critical(const String& message, const String& component = "");

    // Buffer management
    std::vector<LogEntry> getRecentLogs(int count = 50);
    String getLogsAsJson(int count = 50);
    void clearBuffer();
    size_t getBufferSize();

private:
    LogLevel logLevel;
    bool initialized;
    std::vector<LogEntry> logBuffer;

    const char* getLogLevelString(LogLevel level);
    void addToBuffer(LogLevel level, const String& message, const String& component);
    String formatLogEntry(const LogEntry& entry);
};

#endif // LOKI_LOGGER_H