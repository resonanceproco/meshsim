// ============================================================================
// Environment Configuration Loader Implementation
// ============================================================================

#include "../../include/env_config.h"
#include <ArduinoJson.h>

// Global instance
EnvironmentConfig envConfig;

EnvironmentConfig::EnvironmentConfig() : initialized(false) {}

EnvironmentConfig::~EnvironmentConfig() {
    if (initialized) {
        preferences.end();
    }
}

bool EnvironmentConfig::begin() {
    if (!preferences.begin("sim-mesh-cfg", false)) {
        Serial.println("ERROR: Failed to initialize NVS preferences");
        return false;
    }

    initialized = true;
    loadDefaults();
    cacheConfiguration();

    Serial.println("Environment configuration loaded successfully");
    return true;
}

void EnvironmentConfig::loadDefaults() {
    // Only set defaults if keys don't exist
    if (!hasKey("MESH_SSID")) {
        setString("MESH_SSID", EnvDefaults::MESH_SSID);
    }
    if (!hasKey("MESH_PASSWORD")) {
        setString("MESH_PASSWORD", EnvDefaults::MESH_PASSWORD);
    }
    if (!hasKey("MESH_PORT")) {
        setUInt("MESH_PORT", EnvDefaults::MESH_PORT);
    }
    if (!hasKey("MESH_MAX_HOPS")) {
        setUInt("MESH_MAX_HOPS", EnvDefaults::MESH_MAX_HOPS);
    }
    if (!hasKey("MESH_HEARTBEAT_INTERVAL_MS")) {
        setUInt("MESH_HEARTBEAT_INTERVAL_MS", EnvDefaults::MESH_HEARTBEAT_INTERVAL_MS);
    }
    if (!hasKey("LOG_LEVEL")) {
        setString("LOG_LEVEL", EnvDefaults::LOG_LEVEL);
    }
}

void EnvironmentConfig::cacheConfiguration() {
    // Cache frequently accessed values to improve performance
    configCache["MESH_SSID"] = getString("MESH_SSID", EnvDefaults::MESH_SSID);
    configCache["MESH_PASSWORD"] = getString("MESH_PASSWORD", EnvDefaults::MESH_PASSWORD);
    configCache["LOG_LEVEL"] = getString("LOG_LEVEL", EnvDefaults::LOG_LEVEL);
}

String EnvironmentConfig::getString(const char* key, const char* defaultValue) {
    if (!initialized) return String(defaultValue);
    
    // Check cache first
    String keyStr = String(key);
    if (configCache.find(keyStr) != configCache.end()) {
        return configCache[keyStr];
    }

    return preferences.getString(key, defaultValue);
}

int32_t EnvironmentConfig::getInt(const char* key, int32_t defaultValue) {
    if (!initialized) return defaultValue;
    return preferences.getInt(key, defaultValue);
}

uint32_t EnvironmentConfig::getUInt(const char* key, uint32_t defaultValue) {
    if (!initialized) return defaultValue;
    return preferences.getUInt(key, defaultValue);
}

float EnvironmentConfig::getFloat(const char* key, float defaultValue) {
    if (!initialized) return defaultValue;
    return preferences.getFloat(key, defaultValue);
}

bool EnvironmentConfig::getBool(const char* key, bool defaultValue) {
    if (!initialized) return defaultValue;
    return preferences.getBool(key, defaultValue);
}

bool EnvironmentConfig::setString(const char* key, const String& value) {
    if (!initialized) return false;
    
    size_t result = preferences.putString(key, value);
    if (result > 0) {
        // Update cache
        configCache[String(key)] = value;
        return true;
    }
    return false;
}

bool EnvironmentConfig::setInt(const char* key, int32_t value) {
    if (!initialized) return false;
    return preferences.putInt(key, value) > 0;
}

bool EnvironmentConfig::setUInt(const char* key, uint32_t value) {
    if (!initialized) return false;
    return preferences.putUInt(key, value) > 0;
}

bool EnvironmentConfig::setFloat(const char* key, float value) {
    if (!initialized) return false;
    return preferences.putFloat(key, value) > 0;
}

bool EnvironmentConfig::setBool(const char* key, bool value) {
    if (!initialized) return false;
    return preferences.putBool(key, value) > 0;
}

bool EnvironmentConfig::hasKey(const char* key) {
    if (!initialized) return false;
    return preferences.isKey(key);
}

bool EnvironmentConfig::loadAll() {
    if (!initialized) return false;
    
    // Reload cache
    cacheConfiguration();
    Serial.println("Configuration reloaded from NVS");
    return true;
}

bool EnvironmentConfig::saveAll() {
    if (!initialized) return false;
    
    // NVS automatically saves on each set operation
    Serial.println("Configuration saved to NVS");
    return true;
}

bool EnvironmentConfig::resetToDefaults() {
    if (!initialized) return false;
    
    preferences.clear();
    loadDefaults();
    cacheConfiguration();
    
    Serial.println("Configuration reset to defaults");
    return true;
}

String EnvironmentConfig::toJson() {
    DynamicJsonDocument doc(4096);

    // Mesh Network
    doc["mesh"]["ssid"] = getMeshSSID();
    doc["mesh"]["port"] = getMeshPort();
    doc["mesh"]["maxHops"] = getMeshMaxHops();
    doc["mesh"]["heartbeatInterval"] = getMeshHeartbeatInterval();

    // Security (don't export sensitive keys)
    doc["security"]["secureBootEnabled"] = isSecureBootEnabled();
    doc["security"]["encryptionEnabled"] = isEncryptionEnabled();

    // SIM Configuration
    doc["sim"]["totalSlots"] = getTotalSimSlots();
    doc["sim"]["detectionTimeout"] = getSimDetectionTimeout();
    doc["sim"]["healthCheckInterval"] = getSimHealthCheckInterval();
    doc["sim"]["autoDetection"] = isAutoSimDetectionEnabled();

    // GSM Configuration
    doc["gsm"]["baudRate"] = getGSMBaudRate();
    doc["gsm"]["commandTimeout"] = getATCommandTimeout();
    doc["gsm"]["retryAttempts"] = getATCommandRetryAttempts();

    // Health Monitoring
    doc["health"]["checkInterval"] = getHealthCheckInterval();
    doc["health"]["tempWarning"] = getTempWarningThreshold();
    doc["health"]["tempCritical"] = getTempCriticalThreshold();
    doc["health"]["heapWarning"] = getHeapWarningThreshold();
    doc["health"]["heapCritical"] = getHeapCriticalThreshold();

    // Logging
    doc["logging"]["level"] = getLogLevel();
    doc["logging"]["bufferSize"] = getLogBufferSize();
    doc["logging"]["toSerial"] = isLogToSerialEnabled();
    doc["logging"]["toFlash"] = isLogToFlashEnabled();

    // Features
    doc["features"]["persianSMS"] = isPersianSMSEnabled();
    doc["features"]["ota"] = isOTAEnabled();
    doc["features"]["debug"] = isDebugMode();

    String output;
    serializeJson(doc, output);
    return output;
}

bool EnvironmentConfig::fromJson(const String& json) {
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        Serial.println("ERROR: Failed to parse configuration JSON");
        return false;
    }

    // Update configuration from JSON
    if (doc.containsKey("mesh")) {
        JsonObject mesh = doc["mesh"];
        if (mesh.containsKey("ssid")) setString("MESH_SSID", mesh["ssid"].as<String>());
        if (mesh.containsKey("port")) setUInt("MESH_PORT", mesh["port"]);
        if (mesh.containsKey("maxHops")) setUInt("MESH_MAX_HOPS", mesh["maxHops"]);
        if (mesh.containsKey("heartbeatInterval")) setUInt("MESH_HEARTBEAT_INTERVAL_MS", mesh["heartbeatInterval"]);
    }

    if (doc.containsKey("sim")) {
        JsonObject sim = doc["sim"];
        if (sim.containsKey("totalSlots")) setUInt("TOTAL_SIM_SLOTS", sim["totalSlots"]);
        if (sim.containsKey("detectionTimeout")) setUInt("SIM_DETECTION_TIMEOUT_MS", sim["detectionTimeout"]);
        if (sim.containsKey("autoDetection")) setBool("AUTO_SIM_DETECTION_ENABLED", sim["autoDetection"]);
    }

    if (doc.containsKey("logging")) {
        JsonObject logging = doc["logging"];
        if (logging.containsKey("level")) setString("LOG_LEVEL", logging["level"].as<String>());
        if (logging.containsKey("bufferSize")) setUInt("LOG_BUFFER_SIZE", logging["bufferSize"]);
    }

    // Reload cache
    cacheConfiguration();
    Serial.println("Configuration updated from JSON");
    return true;
}
