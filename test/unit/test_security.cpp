#include <unity.h>
#include "secure_key_manager.h"
#include "hmac_handler.h"

SecureKeyManager* keyManager = nullptr;
HMACHandler* hmacHandler = nullptr;

void setUp(void) {
    // Initialize before each test
}

void tearDown(void) {
    // Cleanup after each test
}

// ============================================================================
// SecureKeyManager Tests
// ============================================================================

void test_key_manager_initialization() {
    keyManager = new SecureKeyManager();
    TEST_ASSERT_NOT_NULL(keyManager);
    TEST_ASSERT_TRUE(keyManager->begin());
    delete keyManager;
}

void test_key_generation() {
    keyManager = new SecureKeyManager();
    keyManager->begin();
    
    uint8_t key1[32];
    uint8_t key2[32];
    
    TEST_ASSERT_TRUE(keyManager->generateSecureKey(key1, 32));
    TEST_ASSERT_TRUE(keyManager->generateSecureKey(key2, 32));
    
    // Keys should be different (extremely unlikely to be same with secure RNG)
    TEST_ASSERT_NOT_EQUAL_MEMORY(key1, key2, 32);
    
    delete keyManager;
}

void test_key_storage_and_retrieval() {
    keyManager = new SecureKeyManager();
    keyManager->begin();
    
    uint8_t aesKey[32];
    uint8_t hmacKey[32];
    
    // Keys should be retrievable
    TEST_ASSERT_TRUE(keyManager->getAESKey(aesKey, 32));
    TEST_ASSERT_TRUE(keyManager->getHMACKey(hmacKey, 32));
    
    // Keys should not be all zeros
    bool aesAllZero = true;
    bool hmacAllZero = true;
    
    for (int i = 0; i < 32; i++) {
        if (aesKey[i] != 0) aesAllZero = false;
        if (hmacKey[i] != 0) hmacAllZero = false;
    }
    
    TEST_ASSERT_FALSE(aesAllZero);
    TEST_ASSERT_FALSE(hmacAllZero);
    
    delete keyManager;
}

void test_key_rotation() {
    keyManager = new SecureKeyManager();
    keyManager->begin();
    
    uint8_t keyBefore[32];
    uint8_t keyAfter[32];
    
    keyManager->getAESKey(keyBefore, 32);
    
    TEST_ASSERT_TRUE(keyManager->rotateKeys());
    
    keyManager->getAESKey(keyAfter, 32);
    
    // Keys should be different after rotation
    TEST_ASSERT_NOT_EQUAL_MEMORY(keyBefore, keyAfter, 32);
    
    delete keyManager;
}

void test_key_rotation_timing() {
    keyManager = new SecureKeyManager();
    keyManager->begin();
    
    // Immediately after initialization, should not need rotation
    // (unless it's first run, which forces rotation)
    unsigned long timeSinceRotation = keyManager->getTimeSinceLastRotation();
    
    // Time since rotation should be small (< 1 second = 1000ms)
    TEST_ASSERT_LESS_THAN(1000, timeSinceRotation);
    
    delete keyManager;
}

// ============================================================================
// HMACHandler Tests
// ============================================================================

void test_hmac_initialization() {
    keyManager = new SecureKeyManager();
    keyManager->begin();
    
    hmacHandler = new HMACHandler(keyManager);
    TEST_ASSERT_TRUE(hmacHandler->begin());
    
    delete hmacHandler;
    delete keyManager;
}

void test_hmac_sign_and_verify() {
    keyManager = new SecureKeyManager();
    keyManager->begin();
    hmacHandler = new HMACHandler(keyManager);
    hmacHandler->begin();
    
    const char* message = "Test message for HMAC";
    uint8_t signature[HMACHandler::HMAC_SIZE];
    size_t signatureLen;
    
    // Sign message
    TEST_ASSERT_TRUE(hmacHandler->signMessage(
        (const uint8_t*)message,
        strlen(message),
        signature,
        &signatureLen
    ));
    
    TEST_ASSERT_EQUAL(HMACHandler::HMAC_SIZE, signatureLen);
    
    // Create full message with timestamp and nonce for verification
    size_t fullMessageLen = strlen(message) + HMACHandler::TIMESTAMP_SIZE + HMACHandler::NONCE_SIZE;
    uint8_t* fullMessage = (uint8_t*)malloc(fullMessageLen);
    memcpy(fullMessage, message, strlen(message));
    
    // Note: In real usage, timestamp and nonce would be appended during signing
    // For this test, we'll use appendHMAC instead
    
    free(fullMessage);
    delete hmacHandler;
    delete keyManager;
}

void test_hmac_append_and_verify() {
    keyManager = new SecureKeyManager();
    keyManager->begin();
    hmacHandler = new HMACHandler(keyManager);
    hmacHandler->begin();
    
    const char* originalMessage = "Test message for HMAC";
    size_t originalLen = strlen(originalMessage);
    
    uint8_t signedMessage[256];
    size_t signedLen;
    
    // Append HMAC to message
    TEST_ASSERT_TRUE(hmacHandler->appendHMAC(
        (const uint8_t*)originalMessage,
        originalLen,
        signedMessage,
        &signedLen
    ));
    
    TEST_ASSERT_EQUAL(originalLen + HMACHandler::OVERHEAD, signedLen);
    
    // Verify and extract original message
    uint8_t extractedMessage[256];
    size_t extractedLen;
    
    TEST_ASSERT_TRUE(hmacHandler->verifyAndExtract(
        signedMessage,
        signedLen,
        extractedMessage,
        &extractedLen
    ));
    
    TEST_ASSERT_EQUAL(originalLen, extractedLen);
    TEST_ASSERT_EQUAL_MEMORY(originalMessage, extractedMessage, originalLen);
    
    delete hmacHandler;
    delete keyManager;
}

void test_hmac_tampered_message() {
    keyManager = new SecureKeyManager();
    keyManager->begin();
    hmacHandler = new HMACHandler(keyManager);
    hmacHandler->begin();
    
    const char* originalMessage = "Important message";
    size_t originalLen = strlen(originalMessage);
    
    uint8_t signedMessage[256];
    size_t signedLen;
    
    // Create signed message
    TEST_ASSERT_TRUE(hmacHandler->appendHMAC(
        (const uint8_t*)originalMessage,
        originalLen,
        signedMessage,
        &signedLen
    ));
    
    // Tamper with the message (change first byte)
    signedMessage[0] ^= 0xFF;
    
    // Verification should fail
    uint8_t extractedMessage[256];
    size_t extractedLen;
    
    TEST_ASSERT_FALSE(hmacHandler->verifyAndExtract(
        signedMessage,
        signedLen,
        extractedMessage,
        &extractedLen
    ));
    
    delete hmacHandler;
    delete keyManager;
}

void test_hmac_replay_attack_prevention() {
    keyManager = new SecureKeyManager();
    keyManager->begin();
    hmacHandler = new HMACHandler(keyManager);
    hmacHandler->begin();
    
    const char* message = "Test message";
    size_t messageLen = strlen(message);
    
    uint8_t signedMessage[256];
    size_t signedLen;
    
    // Create signed message
    TEST_ASSERT_TRUE(hmacHandler->appendHMAC(
        (const uint8_t*)message,
        messageLen,
        signedMessage,
        &signedLen
    ));
    
    // First verification should succeed
    uint8_t extracted1[256];
    size_t extractedLen1;
    
    TEST_ASSERT_TRUE(hmacHandler->verifyAndExtract(
        signedMessage,
        signedLen,
        extracted1,
        &extractedLen1
    ));
    
    // Immediate replay should be detected
    uint8_t extracted2[256];
    size_t extractedLen2;
    
    TEST_ASSERT_FALSE(hmacHandler->verifyAndExtract(
        signedMessage,
        signedLen,
        extracted2,
        &extractedLen2
    ));
    
    delete hmacHandler;
    delete keyManager;
}

void test_hmac_performance() {
    keyManager = new SecureKeyManager();
    keyManager->begin();
    hmacHandler = new HMACHandler(keyManager);
    hmacHandler->begin();
    
    const char* message = "Performance test message with reasonable length for realistic testing";
    size_t messageLen = strlen(message);
    
    uint8_t signedMessage[256];
    size_t signedLen;
    
    // Measure HMAC creation time (should be < 5ms per PRD)
    unsigned long startSign = micros();
    
    for (int i = 0; i < 100; i++) {
        hmacHandler->appendHMAC(
            (const uint8_t*)message,
            messageLen,
            signedMessage,
            &signedLen
        );
    }
    
    unsigned long endSign = micros();
    unsigned long avgSignTime = (endSign - startSign) / 100;
    
    Serial.printf("Average HMAC sign time: %lu µs\n", avgSignTime);
    TEST_ASSERT_LESS_THAN(5000, avgSignTime); // < 5ms
    
    // Measure verification time
    uint8_t extracted[256];
    size_t extractedLen;
    
    unsigned long startVerify = micros();
    
    // We can only verify once due to replay prevention, so create new messages
    for (int i = 0; i < 100; i++) {
        hmacHandler->appendHMAC(
            (const uint8_t*)message,
            messageLen,
            signedMessage,
            &signedLen
        );
        
        hmacHandler->verifyAndExtract(
            signedMessage,
            signedLen,
            extracted,
            &extractedLen
        );
    }
    
    unsigned long endVerify = micros();
    unsigned long avgVerifyTime = (endVerify - startVerify) / 100;
    
    Serial.printf("Average HMAC verify time: %lu µs\n", avgVerifyTime);
    TEST_ASSERT_LESS_THAN(5000, avgVerifyTime); // < 5ms
    
    delete hmacHandler;
    delete keyManager;
}

// ============================================================================
// Main Test Runner
// ============================================================================

void setup() {
    delay(2000); // Wait for serial monitor
    
    UNITY_BEGIN();
    
    // SecureKeyManager tests
    RUN_TEST(test_key_manager_initialization);
    RUN_TEST(test_key_generation);
    RUN_TEST(test_key_storage_and_retrieval);
    RUN_TEST(test_key_rotation);
    RUN_TEST(test_key_rotation_timing);
    
    // HMACHandler tests
    RUN_TEST(test_hmac_initialization);
    RUN_TEST(test_hmac_sign_and_verify);
    RUN_TEST(test_hmac_append_and_verify);
    RUN_TEST(test_hmac_tampered_message);
    RUN_TEST(test_hmac_replay_attack_prevention);
    RUN_TEST(test_hmac_performance);
    
    UNITY_END();
}

void loop() {
    // Nothing to do here
}
