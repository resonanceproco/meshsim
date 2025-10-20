// Enterprise SIM Card Mesh Network Node Firmware
#include <Arduino.h>
#include "core/persistent_node_manager.h"
#include "core/configuration_manager.h"
#include "mesh/mesh_network_manager.h"

// Global instances
PersistentNodeManager nodeManager;
ConfigurationManager configManager;
MeshNetworkManager meshManager;

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== Enterprise SIM Mesh Node Starting ===");

    // Initialize core systems
    Serial.println("Initializing persistent node manager...");
    if (!nodeManager.begin()) {
        Serial.println("ERROR: Failed to initialize node manager");
        while (1) delay(1000);
    }

    Serial.println("Initializing configuration manager...");
    if (!configManager.begin()) {
        Serial.println("ERROR: Failed to initialize config manager");
        while (1) delay(1000);
    }

    Serial.println("Initializing mesh network...");
    if (!meshManager.begin()) {
        Serial.println("ERROR: Failed to initialize mesh network");
        while (1) delay(1000);
    }

    // Print node information
    Serial.printf("Node ID: %s\n", nodeManager.getNodeId().c_str());
    Serial.printf("Mesh Node ID: %u\n", meshManager.getNodeId());

    Serial.println("=== Initialization Complete ===");
}

void loop() {
    // Update mesh network
    meshManager.update();

    // Periodic status reporting
    static unsigned long lastStatusReport = 0;
    if (millis() - lastStatusReport > 30000) { // Every 30 seconds
        Serial.printf("Status - Nodes: %d, Connected: %s\n",
                     meshManager.getNodeCount(),
                     meshManager.isNetworkConnected() ? "Yes" : "No");
        lastStatusReport = millis();
    }

    // Small delay to prevent overwhelming the system
    delay(10);
}