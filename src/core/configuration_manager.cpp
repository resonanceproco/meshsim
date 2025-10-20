// Configuration Manager - Manages system settings and persistence
#include <Arduino.h>
#include <ArduinoJson.h>
#include "configuration_manager.h"
#include "../config/mesh_config.h"
#include "../config/security_config.h"
#include "../config/sensor_config.h"

ConfigurationManager::ConfigurationManager() : isLoaded(false) {}

bool ConfigurationManager::begin() {
    loadDefaults();
    isLoaded = true;
    return true;
}

void ConfigurationManager::loadDefaults() {
    // Mesh configuration
    config.mesh.prefix = MESH_PREFIX;
    config.mesh.password = MESH_PASSWORD;
    config.mesh.port = MESH_PORT;
    config.mesh.maxHops = MAX_NETWORK_HOPS;
    config.mesh.heartbeatInterval = HEARTBEAT_INTERVAL;

    // Security configuration
    config.security.encryptionEnabled = true;
    config.security.keyRotationInterval = KEY_ROTATION_INTERVAL;

    // SIM configuration
    config.sim.totalSlots = 20;
    config.sim.detectionTimeout = 5000; // 5 seconds

    // GSM configuration
    config.gsm.signalCheckInterval = 30000; // 30 seconds
    config.gsm.connectionTimeout = 60000;   // 60 seconds

    // System configuration
    config.system.logLevel = 1; // INFO
    config.system.healthCheckInterval = 60000; // 60 seconds
}

bool ConfigurationManager::loadFromJson(const String& jsonString) {
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, jsonString);

    if (error) {
        Serial.println("Failed to parse configuration JSON");
        return false;
    }

    // Parse mesh config
    if (doc.containsKey("mesh")) {
        JsonObject mesh = doc["mesh"];
        config.mesh.prefix = mesh["prefix"] | config.mesh.prefix;
        config.mesh.password = mesh["password"] | config.mesh.password;
        config.mesh.port = mesh["port"] | config.mesh.port;
        config.mesh.maxHops = mesh["maxHops"] | config.mesh.maxHops;
        config.mesh.heartbeatInterval = mesh["heartbeatInterval"] | config.mesh.heartbeatInterval;
    }

    // Parse SIM config
    if (doc.containsKey("sim")) {
        JsonObject sim = doc["sim"];
        config.sim.totalSlots = sim["totalSlots"] | config.sim.totalSlots;
        config.sim.detectionTimeout = sim["detectionTimeout"] | config.sim.detectionTimeout;
    }

    return true;
}

String ConfigurationManager::toJson() {
    DynamicJsonDocument doc(2048);

    // Mesh config
    JsonObject mesh = doc.createNestedObject("mesh");
    mesh["prefix"] = config.mesh.prefix;
    mesh["password"] = config.mesh.password;
    mesh["port"] = config.mesh.port;
    mesh["maxHops"] = config.mesh.maxHops;
    mesh["heartbeatInterval"] = config.mesh.heartbeatInterval;

    // SIM config
    JsonObject sim = doc.createNestedObject("sim");
    sim["totalSlots"] = config.sim.totalSlots;
    sim["detectionTimeout"] = config.sim.detectionTimeout;

    String output;
    serializeJson(doc, output);
    return output;
}

SystemConfig ConfigurationManager::getConfig() {
    return config;
}

bool ConfigurationManager::updateConfig(const SystemConfig& newConfig) {
    config = newConfig;
    return true;
}