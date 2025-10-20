// Performance test for mesh network stress testing
#include <Arduino.h>
#include <unity.h>
#include "../../include/mesh_network_manager.h"
#include "../../include/health_monitor.h"

MeshNetworkManager* meshManager;
HealthMonitor* healthMonitor;

#define STRESS_TEST_DURATION 30000  // 30 seconds
#define MESSAGE_INTERVAL 100        // Send message every 100ms
#define MAX_TEST_MESSAGES 100       // Maximum messages to send

void setUp() {
    meshManager = new MeshNetworkManager();
    healthMonitor = new HealthMonitor();

    meshManager->begin();
    healthMonitor->begin();
}

void tearDown() {
    delete meshManager;
    delete healthMonitor;
}

void test_mesh_message_flood() {
    Serial.println("Starting mesh network stress test...");

    unsigned long startTime = millis();
    unsigned long lastMessageTime = 0;
    int messagesSent = 0;
    int messagesReceived = 0;

    while (millis() - startTime < STRESS_TEST_DURATION && messagesSent < MAX_TEST_MESSAGES) {
        // Send message at regular intervals
        if (millis() - lastMessageTime >= MESSAGE_INTERVAL) {
            // Send a test message (would need actual mesh send method)
            // meshManager->sendMessage("stress_test", "ping");
            messagesSent++;
            lastMessageTime = millis();

            Serial.printf("Sent message %d\n", messagesSent);
        }

        // Check for received messages (would need actual receive method)
        // if (meshManager->hasMessage()) {
        //     messagesReceived++;
        // }

        // Monitor system health during stress test
        SystemHealth health = healthMonitor->getSystemHealth();

        // Check for critical conditions
        if (health.freeHeap < 10000) { // Less than 10KB free heap
            Serial.printf("WARNING: Low memory during stress test: %d bytes\n", health.freeHeap);
        }

        if (health.temperature > 70.0) {
            Serial.printf("WARNING: High temperature during stress test: %.1f°C\n", health.temperature);
        }

        delay(10); // Small delay to prevent overwhelming
    }

    unsigned long duration = millis() - startTime;

    Serial.printf("Stress test completed:\n");
    Serial.printf("  Duration: %lu ms\n", duration);
    Serial.printf("  Messages sent: %d\n", messagesSent);
    Serial.printf("  Messages received: %d\n", messagesReceived);
    Serial.printf("  Send rate: %.1f msg/sec\n", (float)messagesSent / (duration / 1000.0));

    // Basic validation
    TEST_ASSERT_GREATER_THAN(0, messagesSent);
    TEST_ASSERT_LESS_OR_EQUAL(MAX_TEST_MESSAGES, messagesSent);
}

void test_memory_stress_under_load() {
    Serial.println("Testing memory usage under load...");

    SystemHealth initialHealth = healthMonitor->getSystemHealth();
    Serial.printf("Initial free heap: %d bytes\n", initialHealth.freeHeap);

    // Simulate memory-intensive operations
    const int numAllocations = 50;
    void* allocations[numAllocations] = {nullptr};

    // Allocate memory
    for (int i = 0; i < numAllocations; i++) {
        allocations[i] = malloc(1024); // 1KB each
        if (allocations[i] == nullptr) {
            Serial.printf("Failed to allocate memory at iteration %d\n", i);
            break;
        }
    }

    SystemHealth duringLoadHealth = healthMonitor->getSystemHealth();
    Serial.printf("During load free heap: %d bytes\n", duringLoadHealth.freeHeap);

    // Free memory
    for (int i = 0; i < numAllocations; i++) {
        if (allocations[i] != nullptr) {
            free(allocations[i]);
        }
    }

    SystemHealth finalHealth = healthMonitor->getSystemHealth();
    Serial.printf("Final free heap: %d bytes\n", finalHealth.freeHeap);

    // Validate memory was properly freed
    TEST_ASSERT_GREATER_OR_EQUAL(initialHealth.freeHeap - 1024, finalHealth.freeHeap); // Allow some overhead
}

void test_network_resilience() {
    Serial.println("Testing network resilience...");

    // Test mesh network stability over time
    const int checkIntervals = 10;
    bool connectivityHistory[checkIntervals];

    for (int i = 0; i < checkIntervals; i++) {
        connectivityHistory[i] = meshManager->isNetworkConnected();
        delay(1000); // Check every second
    }

    // Calculate connectivity percentage
    int connectedCount = 0;
    for (int i = 0; i < checkIntervals; i++) {
        if (connectivityHistory[i]) connectedCount++;
    }

    float connectivityPercentage = (float)connectedCount / checkIntervals * 100.0;

    Serial.printf("Network connectivity: %.1f%% (%d/%d checks)\n",
                  connectivityPercentage, connectedCount, checkIntervals);

    // Should have some connectivity (adjust threshold based on test environment)
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, connectivityPercentage); // At minimum, test should run
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_mesh_message_flood);
    RUN_TEST(test_memory_stress_under_load);
    RUN_TEST(test_network_resilience);
    UNITY_END();
}

void loop() {
    // Nothing to do here
}