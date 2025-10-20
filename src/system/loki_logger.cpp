// Loki Logger - Structured logging for the system
#include <Arduino.h>
#include "loki_logger.h"

LokiLogger::LokiLogger() : logLevel(LOG_LEVEL_INFO), initialized(false) {}

bool LokiLogger::begin() {
    Serial.println("Loki logger initialized");
    initialized = true;
    return true;
}

void LokiLogger::setLogLevel(LogLevel level) {
    logLevel = level;
}

void LokiLogger::log(LogLevel level, const String& message, const String& component) {
    if (!initialized || level < logLevel) return;

    // Create timestamp
    unsigned long timestamp = millis();
    char timestampStr[20];
    sprintf(timestampStr, "%010lu", timestamp);

    // Create log level string
    const char* levelStr = getLogLevelString(level);

    // Create component string
    String compStr = component.length() > 0 ? component : "SYSTEM";

    // Format log message
    char formattedMessage[512];
    sprintf(formattedMessage, "[%s] %s [%s] %s", timestampStr, levelStr, compStr.c_str(), message.c_str());

    // Output to serial
    Serial.println(formattedMessage);

    // Store in buffer for potential transmission
    addToBuffer(level, message, component);
}

void LokiLogger::debug(const String& message, const String& component) {
    log(LOG_LEVEL_DEBUG, message, component);
}

void LokiLogger::info(const String& message, const String& component) {
    log(LOG_LEVEL_INFO, message, component);
}

void LokiLogger::warning(const String& message, const String& component) {
    log(LOG_LEVEL_WARNING, message, component);
}

void LokiLogger::error(const String& message, const String& component) {
    log(LOG_LEVEL_ERROR, message, component);
}

void LokiLogger::critical(const String& message, const String& component) {
    log(LOG_LEVEL_CRITICAL, message, component);
}

const char* LokiLogger::getLogLevelString(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO: return "INFO";
        case LOG_LEVEL_WARNING: return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_CRITICAL: return "CRIT";
        default: return "UNKNOWN";
    }
}

void LokiLogger::addToBuffer(LogLevel level, const String& message, const String& component) {
    if (logBuffer.size() >= MAX_LOG_BUFFER_SIZE) {
        logBuffer.erase(logBuffer.begin()); // Remove oldest entry
    }

    LogEntry entry;
    entry.timestamp = millis();
    entry.level = level;
    entry.message = message;
    entry.component = component;

    logBuffer.push_back(entry);
}

std::vector<LogEntry> LokiLogger::getRecentLogs(int count) {
    std::vector<LogEntry> recent;

    int start = max(0, (int)logBuffer.size() - count);
    for (size_t i = start; i < logBuffer.size(); i++) {
        recent.push_back(logBuffer[i]);
    }

    return recent;
}

String LokiLogger::getLogsAsJson(int count) {
    std::vector<LogEntry> logs = getRecentLogs(count);

    String json = "{\"logs\":[";

    for (size_t i = 0; i < logs.size(); i++) {
        if (i > 0) json += ",";

        char entry[256];
        sprintf(entry, "{\"timestamp\":%lu,\"level\":\"%s\",\"component\":\"%s\",\"message\":\"%s\"}",
                logs[i].timestamp,
                getLogLevelString(logs[i].level),
                logs[i].component.c_str(),
                logs[i].message.c_str());

        json += entry;
    }

    json += "]}";
    return json;
}

void LokiLogger::clearBuffer() {
    logBuffer.clear();
}

size_t LokiLogger::getBufferSize() {
    return logBuffer.size();
}

String LokiLogger::formatLogEntry(const LogEntry& entry) {
    char formatted[256];
    sprintf(formatted, "[%010lu] %s [%s] %s",
            entry.timestamp,
            getLogLevelString(entry.level),
            entry.component.c_str(),
            entry.message.c_str());

    return String(formatted);
}