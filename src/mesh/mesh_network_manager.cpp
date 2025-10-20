// Mesh Network Manager - Handles painlessMesh networking with AES-256 encryption
#include <Arduino.h>
#include <painlessMesh.h>
#include <AES256.h>
#include "mesh_network_manager.h"
#include "../config/mesh_config.h"
#include "../config/security_config.h"

MeshNetworkManager::MeshNetworkManager() :
    mesh(),
    aes(),
    lastHeartbeat(0),
    nodeCount(0),
    isConnected(false) {}

bool MeshNetworkManager::begin() {
    // Initialize AES encryption
    aes.setKey(AES_KEY, 32);

    // Initialize mesh network
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);

    mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);
    mesh.setContainsRoot(true);

    // Set up callbacks
    mesh.onReceive([this](uint32_t from, String &msg) {
        this->onReceive(from, msg);
    });

    mesh.onNewConnection([this](uint32_t nodeId) {
        this->onNewConnection(nodeId);
    });

    mesh.onDroppedConnection([this](uint32_t nodeId) {
        this->onDroppedConnection(nodeId);
    });

    mesh.onChangedConnections([this]() {
        this->onChangedConnections();
    });

    Serial.println("Mesh network initialized");
    return true;
}

void MeshNetworkManager::update() {
    mesh.update();

    // Send heartbeat every HEARTBEAT_INTERVAL
    if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL) {
        sendHeartbeat();
        lastHeartbeat = millis();
    }
}

bool MeshNetworkManager::sendMessage(uint32_t destId, const String& message, uint8_t hops) {
    if (hops > MAX_NETWORK_HOPS) {
        Serial.println("Message exceeds maximum hop count");
        return false;
    }

    // Encrypt message
    String encryptedMsg = encryptMessage(message);

    // Add hop count and timestamp
    DynamicJsonDocument doc(1024);
    doc["type"] = "data";
    doc["payload"] = encryptedMsg;
    doc["hops"] = hops;
    doc["timestamp"] = millis();
    doc["source"] = mesh.getNodeId();

    String jsonString;
    serializeJson(doc, jsonString);

    return mesh.sendSingle(destId, jsonString);
}

bool MeshNetworkManager::broadcastMessage(const String& message, uint8_t hops) {
    if (hops > MAX_NETWORK_HOPS) {
        Serial.println("Broadcast exceeds maximum hop count");
        return false;
    }

    // Encrypt message
    String encryptedMsg = encryptMessage(message);

    // Add hop count and timestamp
    DynamicJsonDocument doc(1024);
    doc["type"] = "broadcast";
    doc["payload"] = encryptedMsg;
    doc["hops"] = hops;
    doc["timestamp"] = millis();
    doc["source"] = mesh.getNodeId();

    String jsonString;
    serializeJson(doc, jsonString);

    mesh.sendBroadcast(jsonString);
    return true;
}

String MeshNetworkManager::encryptMessage(const String& message) {
    // Simple AES encryption (in production, use proper padding and IV)
    uint8_t block[16];
    message.getBytes(block, 16);

    aes.encryptBlock(block, block);

    // Convert to hex string for transmission
    String encrypted;
    for (int i = 0; i < 16; i++) {
        if (block[i] < 16) encrypted += "0";
        encrypted += String(block[i], HEX);
    }

    return encrypted;
}

String MeshNetworkManager::decryptMessage(const String& encryptedMessage) {
    // Convert hex string back to bytes
    uint8_t block[16];
    for (int i = 0; i < 16; i++) {
        String byteStr = encryptedMessage.substring(i * 2, i * 2 + 2);
        block[i] = strtol(byteStr.c_str(), NULL, 16);
    }

    aes.decryptBlock(block, block);

    return String((char*)block);
}

void MeshNetworkManager::onReceive(uint32_t from, String &msg) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, msg);

    if (error) {
        Serial.println("Failed to parse received message");
        return;
    }

    String type = doc["type"];
    uint8_t hops = doc["hops"] | 0;
    String payload = doc["payload"];

    // Check hop count
    if (hops >= MAX_NETWORK_HOPS) {
        Serial.println("Message discarded: exceeded max hops");
        return;
    }

    // Decrypt payload
    String decryptedPayload = decryptMessage(payload);

    // Handle different message types
    if (type == "heartbeat") {
        handleHeartbeat(from, decryptedPayload);
    } else if (type == "data") {
        handleDataMessage(from, decryptedPayload, hops + 1);
    } else if (type == "command") {
        handleCommand(from, decryptedPayload, hops + 1);
    }
}

void MeshNetworkManager::onNewConnection(uint32_t nodeId) {
    Serial.printf("New connection: %u\n", nodeId);
    nodeCount = mesh.getNodeList().size();
    isConnected = true;
}

void MeshNetworkManager::onDroppedConnection(uint32_t nodeId) {
    Serial.printf("Dropped connection: %u\n", nodeId);
    nodeCount = mesh.getNodeList().size();
    isConnected = (nodeCount > 0);
}

void MeshNetworkManager::onChangedConnections() {
    nodeCount = mesh.getNodeList().size();
    Serial.printf("Connections changed. Total nodes: %d\n", nodeCount);
}

void MeshNetworkManager::sendHeartbeat() {
    DynamicJsonDocument doc(512);
    doc["nodeId"] = mesh.getNodeId();
    doc["uptime"] = millis();
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["nodeCount"] = nodeCount;

    String heartbeatData;
    serializeJson(doc, heartbeatData);

    broadcastMessage(heartbeatData, 0);
}

void MeshNetworkManager::handleHeartbeat(uint32_t from, const String& data) {
    // Process heartbeat data
    Serial.printf("Heartbeat from %u: %s\n", from, data.c_str());
}

void MeshNetworkManager::handleDataMessage(uint32_t from, const String& data, uint8_t hops) {
    // Process data message
    Serial.printf("Data from %u (hops: %d): %s\n", from, hops, data.c_str());
}

void MeshNetworkManager::handleCommand(uint32_t from, const String& command, uint8_t hops) {
    // Process command
    Serial.printf("Command from %u (hops: %d): %s\n", from, hops, command.c_str());
}

uint32_t MeshNetworkManager::getNodeId() {
    return mesh.getNodeId();
}

size_t MeshNetworkManager::getNodeCount() {
    return nodeCount;
}

bool MeshNetworkManager::isNetworkConnected() {
    return isConnected;
}