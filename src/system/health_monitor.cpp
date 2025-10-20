// Health Monitor - System health monitoring and reporting
#include <Arduino.h>
#include "health_monitor.h"

HealthMonitor::HealthMonitor() :
    lastHealthCheck(0),
    consecutiveFailures(0),
    systemHealthy(true) {}

bool HealthMonitor::begin() {
    Serial.println("Health monitor initialized");
    return true;
}

void HealthMonitor::update() {
    // Perform health checks periodically
    if (millis() - lastHealthCheck > HEALTH_CHECK_INTERVAL) {
        performHealthCheck();
        lastHealthCheck = millis();
    }
}

SystemHealth HealthMonitor::getSystemHealth() {
    SystemHealth health;

    // CPU and memory metrics
    health.freeHeap = ESP.getFreeHeap();
    health.heapSize = ESP.getHeapSize();
    health.cpuFreq = ESP.getCpuFreqMHz();

    // Network metrics (would be populated by mesh manager)
    // TODO: Integrate with actual mesh manager to get real metrics
    health.meshConnections = 0; // Placeholder - implement mesh manager integration
    health.networkHealth = 0.0; // Placeholder - calculate based on connection stability

    // SIM metrics (would be populated by SIM manager)
    // TODO: Integrate with actual SIM manager to get real metrics
    health.activeSims = 0; // Placeholder - implement SIM manager integration
    health.simHealth = 0.0; // Placeholder - calculate based on SIM status and signal

    // Power metrics
    health.voltage = readVoltage();
    health.temperature = readTemperature();

    // Overall health score
    health.overallHealth = calculateOverallHealth(health);

    return health;
}

void HealthMonitor::performHealthCheck() {
    SystemHealth health = getSystemHealth();

    // Check critical thresholds
    bool currentHealth = true;

    if (health.freeHeap < CRITICAL_HEAP_THRESHOLD) {
        Serial.printf("CRITICAL: Low heap memory: %d bytes\n", health.freeHeap);
        currentHealth = false;
    }

    if (health.temperature > CRITICAL_TEMPERATURE) {
        Serial.printf("CRITICAL: High temperature: %.1f°C\n", health.temperature);
        currentHealth = false;
    }

    if (health.voltage < CRITICAL_VOLTAGE_MIN || health.voltage > CRITICAL_VOLTAGE_MAX) {
        Serial.printf("CRITICAL: Abnormal voltage: %.2fV\n", health.voltage);
        currentHealth = false;
    }

    // Update health status
    if (!currentHealth) {
        consecutiveFailures++;
        if (consecutiveFailures >= MAX_CONSECUTIVE_FAILURES) {
            systemHealthy = false;
            Serial.println("SYSTEM HEALTH: CRITICAL - Multiple failures detected");
        }
    } else {
        consecutiveFailures = 0;
        systemHealthy = true;
    }

    // Log health summary
    Serial.printf("Health Check - Heap: %d/%d, Temp: %.1f°C, Voltage: %.2fV, Status: %s\n",
                  health.freeHeap, health.heapSize, health.temperature, health.voltage,
                  systemHealthy ? "OK" : "WARNING");
}

float HealthMonitor::calculateOverallHealth(const SystemHealth& health) {
    float score = 1.0;

    // Memory health (40% weight)
    float memoryRatio = (float)health.freeHeap / health.heapSize;
    if (memoryRatio < 0.1) score -= 0.4;      // Critical
    else if (memoryRatio < 0.3) score -= 0.2; // Warning
    else if (memoryRatio < 0.5) score -= 0.1; // Low

    // Temperature health (30% weight)
    if (health.temperature > 70) score -= 0.3;      // Critical
    else if (health.temperature > 60) score -= 0.15; // Warning
    else if (health.temperature > 50) score -= 0.05; // Warm

    // Voltage health (20% weight)
    if (health.voltage < 4.5 || health.voltage > 5.5) score -= 0.2; // Critical
    else if (health.voltage < 4.8 || health.voltage > 5.2) score -= 0.1; // Warning

    // Network health (10% weight)
    score -= (1.0 - health.networkHealth) * 0.1;

    return max(0.0f, min(1.0f, score));
}

float HealthMonitor::readTemperature() {
    // Read temperature from sensor (placeholder)
    // In real implementation, read from temperature sensor
    return 45.0 + (millis() % 20000) / 500.0; // Simulated temperature 45-65°C
}

float HealthMonitor::readVoltage() {
    // Read voltage from ADC (placeholder)
    // In real implementation, read from ADC with voltage divider
    return 5.0 + (sin(millis() / 10000.0) * 0.2); // Simulated voltage 4.8-5.2V
}

String HealthMonitor::getHealthReport() {
    SystemHealth health = getSystemHealth();

    char report[512];
    sprintf(report,
            "System Health Report:\n"
            "  Memory: %d/%d bytes (%.1f%%)\n"
            "  Temperature: %.1f°C\n"
            "  Voltage: %.2fV\n"
            "  CPU Frequency: %d MHz\n"
            "  Network Connections: %d\n"
            "  Active SIMs: %d\n"
            "  Overall Health: %.1f%%\n"
            "  Status: %s",
            health.freeHeap, health.heapSize,
            (float)health.freeHeap / health.heapSize * 100,
            health.temperature, health.voltage, health.cpuFreq,
            health.meshConnections, health.activeSims,
            health.overallHealth * 100,
            systemHealthy ? "HEALTHY" : "WARNING");

    return String(report);
}

bool HealthMonitor::isSystemHealthy() {
    return systemHealthy;
}

void HealthMonitor::resetHealthStatus() {
    consecutiveFailures = 0;
    systemHealthy = true;
    Serial.println("Health status reset");
}