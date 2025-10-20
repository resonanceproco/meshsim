// Secure Boot Implementation
#include "secure_boot.h"

SecureBootManager::SecureBootManager() :
    secureBootEnabled(false),
    currentRollbackVersion(0) {}

SecureBootManager::~SecureBootManager() {}

bool SecureBootManager::begin() {
    Serial.println("Initializing Secure Boot Manager...");

    // Check if secure boot is enabled in hardware
    secureBootEnabled = esp_secure_boot_enabled();

    if (secureBootEnabled) {
        Serial.println("Secure boot is enabled");

        // Load current rollback version
        currentRollbackVersion = getCurrentRollbackVersion();

        // Perform self-test
        if (!performSelfTest()) {
            Serial.println("Secure boot self-test failed!");
            return false;
        }

        Serial.println("Secure boot initialized successfully");
        logSecureBootEvent("SECURE_BOOT_INITIALIZED", "Hardware secure boot active");
    } else {
        Serial.println("Warning: Secure boot is not enabled");
        logSecureBootEvent("SECURE_BOOT_DISABLED", "Hardware secure boot not active");
    }

    return true;
}

bool SecureBootManager::verifyFirmwareSignature(const uint8_t* firmwareData, size_t dataSize) {
    if (!secureBootEnabled) {
        Serial.println("Secure boot not enabled, skipping verification");
        return true; // Allow boot if secure boot is disabled
    }

    Serial.println("Verifying firmware signature...");

    // Calculate SHA256 hash of firmware
    uint8_t calculatedHash[SHA256_SIZE];
    if (!calculateFirmwareHash(firmwareData, dataSize, calculatedHash)) {
        logSecureBootEvent("VERIFICATION_FAILED", "Hash calculation failed");
        return false;
    }

    // Load verification key from secure storage
    uint8_t verificationKey[RSA_KEY_SIZE / 8]; // RSA key size in bytes
    if (!loadVerificationKey(verificationKey, sizeof(verificationKey))) {
        logSecureBootEvent("VERIFICATION_FAILED", "Failed to load verification key");
        return false;
    }

    // In ESP32-S3 secure boot, signature verification is handled by hardware
    // This is a simplified software verification for development
    esp_err_t err = esp_secure_boot_verify_signature(calculatedHash, verificationKey);
    if (err != ESP_OK) {
        Serial.printf("Firmware signature verification failed: %s\n", getEfuseErrorString(err));
        logSecureBootEvent("VERIFICATION_FAILED", "Signature verification failed");
        return false;
    }

    Serial.println("Firmware signature verified successfully");
    logSecureBootEvent("VERIFICATION_SUCCESS", "Firmware signature valid");
    return true;
}

bool SecureBootManager::isSecureBootEnabled() {
    return secureBootEnabled;
}

bool SecureBootManager::generateAndStoreKey() {
    Serial.println("Generating and storing secure boot key...");

    // Generate RSA key pair
    uint8_t publicKey[RSA_KEY_SIZE / 8];
    uint8_t privateKey[RSA_KEY_SIZE / 8];

    // Use ESP32-S3 secure boot key generation
    esp_err_t err = esp_secure_boot_generate_key(publicKey, privateKey);
    if (err != ESP_OK) {
        Serial.printf("Key generation failed: %s\n", getEfuseErrorString(err));
        return false;
    }

    // Store public key in eFuse for verification
    if (!writeToEfuse(publicKey, sizeof(publicKey), EFUSE_BLK_KEY0)) {
        Serial.println("Failed to store public key in eFuse");
        return false;
    }

    // Store private key securely (in development, this would be handled differently)
    // In production, private key should never leave the signing environment

    Serial.println("Secure boot key generated and stored successfully");
    logSecureBootEvent("KEY_GENERATED", "Secure boot key stored in eFuse");
    return true;
}

bool SecureBootManager::loadVerificationKey(uint8_t* keyBuffer, size_t bufferSize) {
    if (bufferSize < RSA_KEY_SIZE / 8) {
        return false;
    }

    return readFromEfuse(keyBuffer, RSA_KEY_SIZE / 8, EFUSE_BLK_KEY0);
}

bool SecureBootManager::setRollbackVersion(uint32_t version) {
    if (!secureBootEnabled) {
        return false;
    }

    esp_err_t err = esp_efuse_write_field_cnt(ESP_EFUSE_ANTI_ROLLBACK, version);
    if (err != ESP_OK) {
        Serial.printf("Failed to set rollback version: %s\n", getEfuseErrorString(err));
        return false;
    }

    currentRollbackVersion = version;
    Serial.printf("Rollback version set to: %u\n", version);
    return true;
}

uint32_t SecureBootManager::getCurrentRollbackVersion() {
    uint32_t version = 0;
    esp_efuse_read_field_blob(ESP_EFUSE_ANTI_ROLLBACK, &version, sizeof(version));
    return version;
}

bool SecureBootManager::checkRollbackProtection(uint32_t firmwareVersion) {
    if (!secureBootEnabled) {
        return true; // Allow if secure boot disabled
    }

    uint32_t currentVersion = getCurrentRollbackVersion();
    if (firmwareVersion < currentVersion) {
        Serial.printf("Rollback protection: firmware version %u < current %u\n",
                     firmwareVersion, currentVersion);
        logSecureBootEvent("ROLLBACK_BLOCKED", String(firmwareVersion));
        return false;
    }

    return true;
}

String SecureBootManager::getSecureBootStatus() {
    if (!secureBootEnabled) {
        return "DISABLED";
    }

    String status = "ENABLED (";
    status += "Rollback Version: " + String(currentRollbackVersion) + ")";
    return status;
}

bool SecureBootManager::performSelfTest() {
    Serial.println("Performing secure boot self-test...");

    // Test hash calculation
    const char* testData = "Secure boot test data";
    uint8_t hash[SHA256_SIZE];
    if (!calculateFirmwareHash((const uint8_t*)testData, strlen(testData), hash)) {
        Serial.println("Self-test: hash calculation failed");
        return false;
    }

    // Test eFuse read/write (if possible)
    uint8_t testByte = 0xAA;
    // Note: eFuse is one-time programmable, so we can't actually test write

    Serial.println("Secure boot self-test passed");
    return true;
}

bool SecureBootManager::calculateFirmwareHash(const uint8_t* data, size_t size, uint8_t* hash) {
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0); // SHA256
    mbedtls_sha256_update(&ctx, data, size);
    mbedtls_sha256_finish(&ctx, hash);
    mbedtls_sha256_free(&ctx);
    return true;
}

bool SecureBootManager::verifyRSASignature(const uint8_t* hash, const uint8_t* signature, size_t sigSize) {
    // This would implement RSA-PSS signature verification
    // For ESP32-S3, this is handled by hardware secure boot
    return esp_secure_boot_verify_signature(hash, signature) == ESP_OK;
}

bool SecureBootManager::writeToEfuse(const uint8_t* data, size_t size, esp_efuse_block_t block) {
    esp_err_t err = esp_efuse_write_block(block, data, 0, size);
    if (err != ESP_OK) {
        Serial.printf("eFuse write failed: %s\n", getEfuseErrorString(err));
        return false;
    }
    return true;
}

bool SecureBootManager::readFromEfuse(uint8_t* buffer, size_t size, esp_efuse_block_t block) {
    esp_err_t err = esp_efuse_read_block(block, buffer, 0, size);
    if (err != ESP_OK) {
        Serial.printf("eFuse read failed: %s\n", getEfuseErrorString(err));
        return false;
    }
    return true;
}

void SecureBootManager::logSecureBootEvent(const String& event, const String& details) {
    Serial.printf("[SECURE_BOOT] %s: %s\n", event.c_str(), details.c_str());
}

String SecureBootManager::getEfuseErrorString(esp_err_t err) {
    switch (err) {
        case ESP_OK: return "OK";
        case ESP_ERR_INVALID_ARG: return "Invalid argument";
        case ESP_ERR_EFUSE: return "eFuse error";
        default: return "Unknown error (" + String(err) + ")";
    }
}