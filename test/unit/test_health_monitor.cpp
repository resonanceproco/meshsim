// Unit test for Health Monitor
#include <Arduino.h>
#include <unity.h>
#include "../../include/health_monitor.h"

HealthMonitor* healthMonitor;

void setUp() {
    healthMonitor = new HealthMonitor();
    healthMonitor->begin();
}

void tearDown() {
    delete healthMonitor;
}

void test_health_monitor_initialization() {
    TEST_ASSERT_TRUE(healthMonitor->begin());
}

void test_system_health_retrieval() {
    SystemHealth health = healthMonitor->getSystemHealth();

    // Check that all health metrics are populated
    TEST_ASSERT_GREATER_THAN(0, health.freeHeap);
    TEST_ASSERT_GREATER_THAN(0, health.heapSize);
    TEST_ASSERT_GREATER_THAN(0, health.cpuFreq);

    // Check voltage is in reasonable range (3.3V - 5.5V)
    TEST_ASSERT_GREATER_THAN(3.0, health.voltage);
    TEST_ASSERT_LESS_THAN(6.0, health.voltage);

    // Check temperature is in reasonable range (0°C - 100°C)
    TEST_ASSERT_GREATER_THAN(0.0, health.temperature);
    TEST_ASSERT_LESS_THAN(100.0, health.temperature);

    // Check overall health is between 0 and 1
    TEST_ASSERT_GREATER_OR_EQUAL(0.0, health.overallHealth);
    TEST_ASSERT_LESS_OR_EQUAL(1.0, health.overallHealth);
}

void test_health_report_generation() {
    String report = healthMonitor->getHealthReport();

    TEST_ASSERT_TRUE(report.length() > 0);
    TEST_ASSERT_TRUE(report.indexOf("System Health Report") >= 0);
    TEST_ASSERT_TRUE(report.indexOf("Memory:") >= 0);
    TEST_ASSERT_TRUE(report.indexOf("Temperature:") >= 0);
    TEST_ASSERT_TRUE(report.indexOf("Voltage:") >= 0);
    TEST_ASSERT_TRUE(report.indexOf("Overall Health:") >= 0);
}

void test_system_healthy_status() {
    // Initially should be healthy
    TEST_ASSERT_TRUE(healthMonitor->isSystemHealthy());

    // Reset status
    healthMonitor->resetHealthStatus();
    TEST_ASSERT_TRUE(healthMonitor->isSystemHealthy());
}

void test_health_check_execution() {
    // Perform health check
    healthMonitor->update();

    // Should still be healthy after one check
    TEST_ASSERT_TRUE(healthMonitor->isSystemHealthy());
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_health_monitor_initialization);
    RUN_TEST(test_system_health_retrieval);
    RUN_TEST(test_health_report_generation);
    RUN_TEST(test_system_healthy_status);
    RUN_TEST(test_health_check_execution);
    UNITY_END();
}

void loop() {
    // Nothing to do here
}