// Configuration Manager Header
#ifndef CONFIGURATION_MANAGER_H
#define CONFIGURATION_MANAGER_H

#include <Arduino.h>
#include <WString.h>

// Configuration structures
struct MeshConfig {
    String prefix;
    String password;
    uint16_t port;
    uint8_t maxHops;
    uint32_t heartbeatInterval;
};

struct SecurityConfig {
    bool encryptionEnabled;
    uint32_t keyRotationInterval;
};

struct SIMConfig {
    uint8_t totalSlots;
    uint32_t detectionTimeout;
};

struct GSMConfig {
    uint32_t signalCheckInterval;
    uint32_t connectionTimeout;
};

struct SystemConfig {
    uint8_t logLevel;
    uint32_t healthCheckInterval;
    MeshConfig mesh;
    SecurityConfig security;
    SIMConfig sim;
    GSMConfig gsm;
};

class ConfigurationManager {
public:
    ConfigurationManager();
    bool begin();
    bool loadFromJson(const String& jsonString);
    String toJson();
    SystemConfig getConfig();
    bool updateConfig(const SystemConfig& newConfig);

private:
    SystemConfig config;
    bool isLoaded;

    void loadDefaults();
};

#endif // CONFIGURATION_MANAGER_H