// Unit test for AES-256 encryption
#include <Arduino.h>
#include <unity.h>
#include "../../include/aes256_encryption.h"

AES256Encryption* aes;

void setUp() {
    aes = new AES256Encryption();
    aes->begin();
}

void tearDown() {
    delete aes;
}

void test_aes_encryption_decryption() {
    const char* testMessage = "Hello, World!";
    String encrypted = aes->encrypt(testMessage);
    String decrypted = aes->decrypt(encrypted);

    TEST_ASSERT_EQUAL_STRING(testMessage, decrypted.c_str());
}

void test_aes_hmac_generation() {
    const char* testMessage = "Test message";
    String hmac1 = aes->generateHMAC(testMessage);
    String hmac2 = aes->generateHMAC(testMessage);

    // HMAC should be consistent for same input
    TEST_ASSERT_EQUAL_STRING(hmac1.c_str(), hmac2.c_str());

    // HMAC should be different for different input
    String hmac3 = aes->generateHMAC("Different message");
    TEST_ASSERT_NOT_EQUAL(hmac1, hmac3);
}

void test_aes_hmac_verification() {
    const char* testMessage = "Test message";
    String hmac = aes->generateHMAC(testMessage);

    TEST_ASSERT_TRUE(aes->verifyHMAC(testMessage, hmac));
    TEST_ASSERT_FALSE(aes->verifyHMAC("Wrong message", hmac));
}

void test_aes_key_rotation() {
    const char* testMessage = "Test message";
    String encrypted1 = aes->encrypt(testMessage);

    aes->rotateKey();

    String encrypted2 = aes->encrypt(testMessage);

    // Encryption should be different after key rotation
    TEST_ASSERT_NOT_EQUAL(encrypted1, encrypted2);

    // But should still be decryptable
    String decrypted = aes->decrypt(encrypted2);
    TEST_ASSERT_EQUAL_STRING(testMessage, decrypted.c_str());
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_aes_encryption_decryption);
    RUN_TEST(test_aes_hmac_generation);
    RUN_TEST(test_aes_hmac_verification);
    RUN_TEST(test_aes_key_rotation);
    UNITY_END();
}

void loop() {
    // Nothing to do here
}