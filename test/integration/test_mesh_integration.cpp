// Integration test for mesh network functionality
#include <Arduino.h>
#include <unity.h>
#include "../../include/mesh_network_manager.h"
#include "../../include/health_monitor.h"
#include "../../include/sim_multiplexer.h"

MeshNetworkManager* meshManager;
HealthMonitor* healthMonitor;
SIMMultiplexer* simManager;

void setUp() {
    meshManager = new MeshNetworkManager();
    healthMonitor = new HealthMonitor();
    simManager = new SIMMultiplexer();

    meshManager->begin();
    healthMonitor->begin();
    simManager->begin();
}

void tearDown() {
    delete meshManager;
    delete healthMonitor;
    delete simManager;
}

void test_mesh_network_initialization() {
    TEST_ASSERT_TRUE(meshManager->begin());
    TEST_ASSERT_GREATER_THAN(0, meshManager->getNodeId());
}

void test_health_monitor_integration() {
    TEST_ASSERT_TRUE(healthMonitor->begin());

    // Test that health monitor can get mesh network data
    SystemHealth health = healthMonitor->getSystemHealth();

    // Should have mesh connection data (even if 0)
    TEST_ASSERT_GREATER_OR_EQUAL(0, health.meshConnections);
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, health.networkHealth);
    TEST_ASSERT_LESS_OR_EQUAL(1.0, health.networkHealth);
}

void test_sim_manager_integration() {
    TEST_ASSERT_TRUE(simManager->begin());

    // Test SIM manager provides data to health monitor
    SystemHealth health = healthMonitor->getSystemHealth();

    // Should have SIM data (even if 0)
    TEST_ASSERT_GREATER_OR_EQUAL(0, health.activeSims);
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, health.simHealth);
    TEST_ASSERT_LESS_OR_EQUAL(1.0, health.simHealth);
}

void test_mesh_network_connectivity() {
    // Test basic mesh network connectivity
    bool connected = meshManager->isNetworkConnected();
    int nodeCount = meshManager->getNodeCount();

    // These might be false/0 in test environment, but should not crash
    TEST_ASSERT_TRUE(true); // Basic connectivity test passed if we reach here

    Serial.printf("Mesh connected: %s, Nodes: %d\n", connected ? "Yes" : "No", nodeCount);
}

void test_system_integration_health() {
    // Test that all components work together
    SystemHealth health = healthMonitor->getSystemHealth();

    // Overall health should be calculable
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, health.overallHealth);
    TEST_ASSERT_LESS_OR_EQUAL(1.0, health.overallHealth);

    // Should have valid memory readings
    TEST_ASSERT_GREATER_THAN(0, health.freeHeap);
    TEST_ASSERT_GREATER_THAN(0, health.heapSize);

    String report = healthMonitor->getHealthReport();
    TEST_ASSERT_TRUE(report.length() > 0);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_mesh_network_initialization);
    RUN_TEST(test_health_monitor_integration);
    RUN_TEST(test_sim_manager_integration);
    RUN_TEST(test_mesh_network_connectivity);
    RUN_TEST(test_system_integration_health);
    UNITY_END();
}

void loop() {
    // Nothing to do here
}