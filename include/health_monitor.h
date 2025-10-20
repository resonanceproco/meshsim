// Health Monitor Header
#ifndef HEALTH_MONITOR_H
#define HEALTH_MONITOR_H

#include <Arduino.h>

#define HEALTH_CHECK_INTERVAL 60000      // 60 seconds
#define CRITICAL_HEAP_THRESHOLD 50000    // 50KB
#define CRITICAL_TEMPERATURE 75.0        // 75°C
#define CRITICAL_VOLTAGE_MIN 4.5         // 4.5V
#define CRITICAL_VOLTAGE_MAX 5.5         // 5.5V
#define MAX_CONSECUTIVE_FAILURES 3       // Max failures before critical status

struct SystemHealth {
    // Memory
    uint32_t freeHeap;
    uint32_t heapSize;

    // CPU
    uint32_t cpuFreq;

    // Network
    uint32_t meshConnections;
    float networkHealth; // 0.0 to 1.0

    // SIM cards
    uint32_t activeSims;
    float simHealth; // 0.0 to 1.0

    // Power
    float voltage;
    float temperature;

    // Overall
    float overallHealth; // 0.0 to 1.0
};

class HealthMonitor {
public:
    HealthMonitor();
    bool begin();
    void update();

    SystemHealth getSystemHealth();
    String getHealthReport();
    bool isSystemHealthy();
    void resetHealthStatus();

private:
    unsigned long lastHealthCheck;
    uint32_t consecutiveFailures;
    bool systemHealthy;

    void performHealthCheck();
    float calculateOverallHealth(const SystemHealth& health);
    float readTemperature();
    float readVoltage();
};

#endif // HEALTH_MONITOR_H