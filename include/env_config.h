// ============================================================================
// Environment Configuration Loader for ESP32 Firmware
// ============================================================================
// This header provides runtime configuration loading from environment variables
// stored in NVS (Non-Volatile Storage)

#ifndef ENV_CONFIG_H
#define ENV_CONFIG_H

#include <Arduino.h>
#include <Preferences.h>
#include <map>

// Default values if not set in NVS
namespace EnvDefaults {
    // Mesh Network
    const char* MESH_SSID = "SIM_MESH";
    const char* MESH_PASSWORD = "sim_mesh_secure_2024";
    const uint16_t MESH_PORT = 5555;
    const uint8_t MESH_MAX_HOPS = 6;
    const uint32_t MESH_HEARTBEAT_INTERVAL_MS = 10000;

    // Security
    const char* AES_KEY = "0001020304050607080910111213141516171819202122232425262728293031";
    const char* HMAC_KEY = "2021222324252627282930313233343536373839404142434445464748495051";
    const uint32_t KEY_ROTATION_INTERVAL_MS = 86400000;

    // SIM Configuration
    const uint8_t TOTAL_SIM_SLOTS = 20;
    const uint32_t SIM_DETECTION_TIMEOUT_MS = 5000;
    const uint32_t SIM_HEALTH_CHECK_INTERVAL_MS = 300000;

    // GSM Configuration
    const uint32_t GSM_BAUD_RATE = 115200;
    const uint32_t AT_COMMAND_TIMEOUT_MS = 5000;

    // Health Monitoring
    const uint32_t HEALTH_CHECK_INTERVAL_MS = 60000;
    const float TEMP_WARNING_THRESHOLD = 60.0;
    const float TEMP_CRITICAL_THRESHOLD = 75.0;

    // Logging
    const char* LOG_LEVEL = "INFO";
    const uint16_t LOG_BUFFER_SIZE = 100;
}

class EnvironmentConfig {
public:
    EnvironmentConfig();
    ~EnvironmentConfig();

    // Initialize configuration system
    bool begin();

    // String configuration getters
    String getString(const char* key, const char* defaultValue = "");
    
    // Integer configuration getters
    int32_t getInt(const char* key, int32_t defaultValue = 0);
    uint32_t getUInt(const char* key, uint32_t defaultValue = 0);
    
    // Float configuration getters
    float getFloat(const char* key, float defaultValue = 0.0);
    
    // Boolean configuration getters
    bool getBool(const char* key, bool defaultValue = false);

    // Configuration setters (saves to NVS)
    bool setString(const char* key, const String& value);
    bool setInt(const char* key, int32_t value);
    bool setUInt(const char* key, uint32_t value);
    bool setFloat(const char* key, float value);
    bool setBool(const char* key, bool value);

    // Load all configuration from NVS
    bool loadAll();

    // Save current configuration to NVS
    bool saveAll();

    // Reset to defaults
    bool resetToDefaults();

    // Check if configuration exists
    bool hasKey(const char* key);

    // Get all configuration as JSON
    String toJson();

    // Load configuration from JSON
    bool fromJson(const String& json);

    // Mesh Network Config
    String getMeshSSID() { return getString("MESH_SSID", EnvDefaults::MESH_SSID); }
    String getMeshPassword() { return getString("MESH_PASSWORD", EnvDefaults::MESH_PASSWORD); }
    uint16_t getMeshPort() { return getUInt("MESH_PORT", EnvDefaults::MESH_PORT); }
    uint8_t getMeshMaxHops() { return getUInt("MESH_MAX_HOPS", EnvDefaults::MESH_MAX_HOPS); }
    uint32_t getMeshHeartbeatInterval() { return getUInt("MESH_HEARTBEAT_INTERVAL_MS", EnvDefaults::MESH_HEARTBEAT_INTERVAL_MS); }

    // Security Config
    String getAESKey() { return getString("AES_ENCRYPTION_KEY", EnvDefaults::AES_KEY); }
    String getHMACKey() { return getString("HMAC_KEY", EnvDefaults::HMAC_KEY); }
    uint32_t getKeyRotationInterval() { return getUInt("KEY_ROTATION_INTERVAL_MS", EnvDefaults::KEY_ROTATION_INTERVAL_MS); }
    bool isSecureBootEnabled() { return getBool("SECURE_BOOT_ENABLED", true); }

    // SIM Config
    uint8_t getTotalSimSlots() { return getUInt("TOTAL_SIM_SLOTS", EnvDefaults::TOTAL_SIM_SLOTS); }
    uint32_t getSimDetectionTimeout() { return getUInt("SIM_DETECTION_TIMEOUT_MS", EnvDefaults::SIM_DETECTION_TIMEOUT_MS); }
    uint32_t getSimHealthCheckInterval() { return getUInt("SIM_HEALTH_CHECK_INTERVAL_MS", EnvDefaults::SIM_HEALTH_CHECK_INTERVAL_MS); }
    bool isAutoSimDetectionEnabled() { return getBool("AUTO_SIM_DETECTION_ENABLED", true); }

    // GSM Config
    uint32_t getGSMBaudRate() { return getUInt("GSM_BAUD_RATE", EnvDefaults::GSM_BAUD_RATE); }
    uint32_t getATCommandTimeout() { return getUInt("AT_COMMAND_TIMEOUT_MS", EnvDefaults::AT_COMMAND_TIMEOUT_MS); }
    uint8_t getATCommandRetryAttempts() { return getUInt("AT_COMMAND_RETRY_ATTEMPTS", 3); }

    // Health Monitoring Config
    uint32_t getHealthCheckInterval() { return getUInt("HEALTH_CHECK_INTERVAL_MS", EnvDefaults::HEALTH_CHECK_INTERVAL_MS); }
    float getTempWarningThreshold() { return getFloat("TEMP_WARNING_THRESHOLD", EnvDefaults::TEMP_WARNING_THRESHOLD); }
    float getTempCriticalThreshold() { return getFloat("TEMP_CRITICAL_THRESHOLD", EnvDefaults::TEMP_CRITICAL_THRESHOLD); }
    uint32_t getHeapWarningThreshold() { return getUInt("HEAP_WARNING_THRESHOLD", 100000); }
    uint32_t getHeapCriticalThreshold() { return getUInt("HEAP_CRITICAL_THRESHOLD", 50000); }

    // Logging Config
    String getLogLevel() { return getString("LOG_LEVEL", EnvDefaults::LOG_LEVEL); }
    uint16_t getLogBufferSize() { return getUInt("LOG_BUFFER_SIZE", EnvDefaults::LOG_BUFFER_SIZE); }
    bool isLogToSerialEnabled() { return getBool("LOG_TO_SERIAL", true); }
    bool isLogToFlashEnabled() { return getBool("LOG_TO_FLASH", true); }

    // Server Connection Config (Master Node)
    String getServerURL() { return getString("SERVER_URL", ""); }
    String getMQTTBrokerURL() { return getString("MQTT_BROKER_URL", ""); }
    uint16_t getMQTTBrokerPort() { return getUInt("MQTT_BROKER_PORT", 1883); }
    String getMQTTUsername() { return getString("MQTT_USERNAME", ""); }
    String getMQTTPassword() { return getString("MQTT_PASSWORD", ""); }

    // Feature Flags
    bool isPersianSMSEnabled() { return getBool("ENABLE_PERSIAN_SMS", true); }
    bool isEncryptionEnabled() { return getBool("ENABLE_ENCRYPTION", true); }
    bool isOTAEnabled() { return getBool("OTA_ENABLED", true); }
    bool isDebugMode() { return getBool("DEBUG_MODE", false); }

private:
    Preferences preferences;
    bool initialized;
    std::map<String, String> configCache;

    // Helper methods
    void loadDefaults();
    void cacheConfiguration();
};

// Global instance
extern EnvironmentConfig envConfig;

#endif // ENV_CONFIG_H
