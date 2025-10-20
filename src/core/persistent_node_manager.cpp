// Persistent Node Manager - Manages unique node identity across reboots
#include <Arduino.h>
#include <EEPROM.h>
#include "persistent_node_manager.h"

#define EEPROM_SIZE 512
#define NODE_ID_OFFSET 0
#define NODE_ID_SIZE 16
#define CONFIG_OFFSET 16
#define CONFIG_SIZE 64

PersistentNodeManager::PersistentNodeManager() : nodeId(""), isInitialized(false) {}

bool PersistentNodeManager::begin() {
    EEPROM.begin(EEPROM_SIZE);

    // Generate or load persistent node ID
    if (!loadNodeId()) {
        generateNodeId();
        saveNodeId();
    }

    isInitialized = true;
    return true;
}

String PersistentNodeManager::getNodeId() {
    return nodeId;
}

bool PersistentNodeManager::loadNodeId() {
    char buffer[NODE_ID_SIZE + 1];
    for (int i = 0; i < NODE_ID_SIZE; i++) {
        buffer[i] = EEPROM.read(NODE_ID_OFFSET + i);
    }
    buffer[NODE_ID_SIZE] = '\0';

    // Check if valid node ID exists
    if (buffer[0] != '\0' && strlen(buffer) > 0) {
        nodeId = String(buffer);
        return true;
    }

    return false;
}

void PersistentNodeManager::generateNodeId() {
    // Generate unique ID from hardware fingerprints
    uint32_t chipId = ESP.getEfuseMac();
    uint32_t flashId = ESP.getFlashChipId();

    char idBuffer[17];
    sprintf(idBuffer, "%08X%08X", chipId, flashId);
    nodeId = String(idBuffer);
}

void PersistentNodeManager::saveNodeId() {
    for (int i = 0; i < NODE_ID_SIZE && i < nodeId.length(); i++) {
        EEPROM.write(NODE_ID_OFFSET + i, nodeId.charAt(i));
    }
    EEPROM.commit();
}

bool PersistentNodeManager::saveConfig(const String& config) {
    if (config.length() > CONFIG_SIZE) return false;

    for (int i = 0; i < CONFIG_SIZE; i++) {
        char c = (i < config.length()) ? config.charAt(i) : '\0';
        EEPROM.write(CONFIG_OFFSET + i, c);
    }
    EEPROM.commit();
    return true;
}

String PersistentNodeManager::loadConfig() {
    char buffer[CONFIG_SIZE + 1];
    for (int i = 0; i < CONFIG_SIZE; i++) {
        buffer[i] = EEPROM.read(CONFIG_OFFSET + i);
    }
    buffer[CONFIG_SIZE] = '\0';
    return String(buffer);
}