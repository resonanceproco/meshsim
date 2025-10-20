// Watchdog Timer Header
#ifndef WATCHDOG_TIMER_H
#define WATCHDOG_TIMER_H

#include <Arduino.h>
#include <esp_task_wdt.h>

/**
 * @brief Hardware Watchdog Timer for ESP32-S3
 *
 * Implements dual watchdog functionality:
 * 1. Task Watchdog Timer (TWDT) - monitors individual tasks
 * 2. Interrupt Watchdog Timer (IWDT) - monitors interrupt routines
 *
 * PRD Compliance:
 * - REQ-FW-201: Hardware watchdog with 30s timeout
 * - REQ-FW-202: Automatic system reset on watchdog timeout
 * - REQ-FW-203: Watchdog feed mechanism for healthy tasks
 */
class WatchdogTimer {
public:
    WatchdogTimer();
    ~WatchdogTimer();

    // Initialization
    bool begin(uint32_t timeoutSeconds = 30);

    // Task monitoring
    bool addTask(TaskHandle_t taskHandle, const char* taskName = nullptr);
    bool removeTask(TaskHandle_t taskHandle);
    bool feedTask(TaskHandle_t taskHandle = nullptr); // nullptr = current task

    // System monitoring
    void feedSystem();
    bool isSystemHealthy();

    // Configuration
    void setTimeout(uint32_t timeoutSeconds);
    uint32_t getTimeout();

    // Status
    void printStatus();

private:
    uint32_t timeoutMs;
    bool initialized;
    TaskHandle_t mainTaskHandle;

    void initializeTWDT();
    void initializeIWDT();
    void systemReset();
};

#endif // WATCHDOG_TIMER_H