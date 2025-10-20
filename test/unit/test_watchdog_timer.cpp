// Unit test for Watchdog Timer
#include <Arduino.h>
#include <unity.h>
#include "../../include/watchdog_timer.h"

WatchdogTimer* watchdog;

void setUp() {
    watchdog = new WatchdogTimer();
}

void tearDown() {
    delete watchdog;
}

void test_watchdog_initialization() {
    TEST_ASSERT_TRUE(watchdog->begin(10)); // 10 second timeout for testing
}

void test_watchdog_timeout_configuration() {
    watchdog->begin(30);
    TEST_ASSERT_EQUAL(30, watchdog->getTimeout());

    watchdog->setTimeout(60);
    TEST_ASSERT_EQUAL(60, watchdog->getTimeout());
}

void test_watchdog_feeding() {
    watchdog->begin(10);

    // Feed the watchdog
    TEST_ASSERT_TRUE(watchdog->feedSystem());
    TEST_ASSERT_TRUE(watchdog->feedTask(nullptr)); // Feed current task
}

void test_watchdog_system_health() {
    watchdog->begin(10);

    // Initially should be healthy
    TEST_ASSERT_TRUE(watchdog->isSystemHealthy());

    // After feeding, should still be healthy
    watchdog->feedSystem();
    TEST_ASSERT_TRUE(watchdog->isSystemHealthy());
}

void test_watchdog_task_management() {
    watchdog->begin(10);

    // Get current task handle
    TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();

    // Add current task to watchdog
    TEST_ASSERT_TRUE(watchdog->addTask(currentTask, "TestTask"));

    // Feed the task
    TEST_ASSERT_TRUE(watchdog->feedTask(currentTask));

    // Remove task from watchdog
    TEST_ASSERT_TRUE(watchdog->removeTask(currentTask));
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_watchdog_initialization);
    RUN_TEST(test_watchdog_timeout_configuration);
    RUN_TEST(test_watchdog_feeding);
    RUN_TEST(test_watchdog_system_health);
    RUN_TEST(test_watchdog_task_management);
    UNITY_END();
}

void loop() {
    // Nothing to do here
}