// Watchdog Timer Implementation
#include "watchdog_timer.h"

WatchdogTimer::WatchdogTimer() :
    timeoutMs(30000), // 30 seconds default
    initialized(false),
    mainTaskHandle(nullptr) {}

WatchdogTimer::~WatchdogTimer() {
    if (initialized) {
        esp_task_wdt_delete(nullptr);
    }
}

bool WatchdogTimer::begin(uint32_t timeoutSeconds) {
    timeoutMs = timeoutSeconds * 1000;

    // Initialize Task Watchdog Timer
    initializeTWDT();

    // Initialize Interrupt Watchdog Timer
    initializeIWDT();

    initialized = true;
    Serial.println("Watchdog timer initialized");

    // Get main task handle for monitoring
    mainTaskHandle = xTaskGetCurrentTaskHandle();

    return true;
}

void WatchdogTimer::initializeTWDT() {
    // Configure TWDT
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = timeoutMs,
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1, // Monitor all cores
        .trigger_panic = true // Reset on timeout
    };

    esp_err_t err = esp_task_wdt_init(&twdt_config);
    if (err != ESP_OK) {
        Serial.printf("TWDT init failed: %s\n", esp_err_to_name(err));
        return;
    }

    // Add current task to watchdog
    err = esp_task_wdt_add(mainTaskHandle);
    if (err != ESP_OK) {
        Serial.printf("TWDT add task failed: %s\n", esp_err_to_name(err));
    }
}

void WatchdogTimer::initializeIWDT() {
    // IWDT is automatically enabled by ESP-IDF
    // Configure timeout
    esp_err_t err = esp_task_wdt_init((esp_task_wdt_config_t){
        .timeout_ms = timeoutMs,
        .idle_core_mask = 0, // IWDT doesn't monitor idle tasks
        .trigger_panic = true
    });

    if (err != ESP_OK) {
        Serial.printf("IWDT config failed: %s\n", esp_err_to_name(err));
    }
}

bool WatchdogTimer::addTask(TaskHandle_t taskHandle, const char* taskName) {
    if (!initialized) return false;

    esp_err_t err = esp_task_wdt_add(taskHandle);
    if (err != ESP_OK) {
        Serial.printf("Failed to add task to WDT: %s\n", esp_err_to_name(err));
        return false;
    }

    if (taskName) {
        Serial.printf("Task '%s' added to watchdog monitoring\n", taskName);
    }

    return true;
}

bool WatchdogTimer::removeTask(TaskHandle_t taskHandle) {
    if (!initialized) return false;

    esp_err_t err = esp_task_wdt_delete(taskHandle);
    if (err != ESP_OK) {
        Serial.printf("Failed to remove task from WDT: %s\n", esp_err_to_name(err));
        return false;
    }

    return true;
}

bool WatchdogTimer::feedTask(TaskHandle_t taskHandle) {
    if (!initialized) return false;

    TaskHandle_t handle = taskHandle ? taskHandle : xTaskGetCurrentTaskHandle();
    esp_err_t err = esp_task_wdt_reset();

    if (err != ESP_OK) {
        Serial.printf("WDT feed failed: %s\n", esp_err_to_name(err));
        return false;
    }

    return true;
}

void WatchdogTimer::feedSystem() {
    feedTask(nullptr); // Feed current task
}

bool WatchdogTimer::isSystemHealthy() {
    // Check if watchdog is still active (system hasn't reset)
    return initialized;
}

void WatchdogTimer::setTimeout(uint32_t timeoutSeconds) {
    timeoutMs = timeoutSeconds * 1000;

    if (initialized) {
        // Reconfigure with new timeout
        esp_task_wdt_config_t config = {
            .timeout_ms = timeoutMs,
            .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
            .trigger_panic = true
        };

        esp_task_wdt_init(&config);
    }
}

uint32_t WatchdogTimer::getTimeout() {
    return timeoutMs / 1000;
}

void WatchdogTimer::printStatus() {
    Serial.printf("Watchdog Status:\n");
    Serial.printf("  Initialized: %s\n", initialized ? "Yes" : "No");
    Serial.printf("  Timeout: %u seconds\n", getTimeout());
    Serial.printf("  System Healthy: %s\n", isSystemHealthy() ? "Yes" : "No");
}