// Mesh Network Manager Header
#ifndef MESH_NETWORK_MANAGER_H
#define MESH_NETWORK_MANAGER_H

#include <Arduino.h>
#include <painlessMesh.h>
#include <AES256.h>
#include <ArduinoJson.h>

class MeshNetworkManager {
public:
    MeshNetworkManager();
    bool begin();
    void update();
    bool sendMessage(uint32_t destId, const String& message, uint8_t hops = 0);
    bool broadcastMessage(const String& message, uint8_t hops = 0);
    uint32_t getNodeId();
    size_t getNodeCount();
    bool isNetworkConnected();

private:
    painlessMesh mesh;
    AES256 aes;
    unsigned long lastHeartbeat;
    size_t nodeCount;
    bool isConnected;

    String encryptMessage(const String& message);
    String decryptMessage(const String& encryptedMessage);

    // Callback handlers
    void onReceive(uint32_t from, String &msg);
    void onNewConnection(uint32_t nodeId);
    void onDroppedConnection(uint32_t nodeId);
    void onChangedConnections();

    // Message handlers
    void sendHeartbeat();
    void handleHeartbeat(uint32_t from, const String& data);
    void handleDataMessage(uint32_t from, const String& data, uint8_t hops);
    void handleCommand(uint32_t from, const String& command, uint8_t hops);
};

#endif // MESH_NETWORK_MANAGER_H