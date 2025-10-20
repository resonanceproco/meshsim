// Secure Boot Header
#ifndef SECURE_BOOT_H
#define SECURE_BOOT_H

#include <Arduino.h>
#include <esp_secure_boot.h>
#include <esp_efuse.h>
#include <mbedtls/sha256.h>

/**
 * @brief Secure Boot Manager for ESP32-S3
 *
 * Implements secure boot functionality to ensure only signed firmware runs.
 * Uses ESP32-S3 hardware secure boot features with RSA-PSS signature verification.
 *
 * PRD Compliance:
 * - REQ-FW-305: Hardware-based secure boot
 * - REQ-FW-306: RSA signature verification
 * - REQ-FW-307: Secure key storage in eFuse
 * - REQ-FW-308: Anti-rollback protection
 */
class SecureBootManager {
public:
    SecureBootManager();
    ~SecureBootManager();

    // Initialization
    bool begin();

    // Secure boot verification
    bool verifyFirmwareSignature(const uint8_t* firmwareData, size_t dataSize);
    bool isSecureBootEnabled();

    // Key management
    bool generateAndStoreKey();
    bool loadVerificationKey(uint8_t* keyBuffer, size_t bufferSize);

    // Anti-rollback
    bool setRollbackVersion(uint32_t version);
    uint32_t getCurrentRollbackVersion();
    bool checkRollbackProtection(uint32_t firmwareVersion);

    // Status and diagnostics
    String getSecureBootStatus();
    bool performSelfTest();

private:
    bool secureBootEnabled;
    uint32_t currentRollbackVersion;

    // RSA key parameters (for ESP32-S3 secure boot)
    static const size_t RSA_KEY_SIZE = 3072; // 3072-bit RSA
    static const size_t SHA256_SIZE = 32;

    // Secure boot verification
    bool calculateFirmwareHash(const uint8_t* data, size_t size, uint8_t* hash);
    bool verifyRSASignature(const uint8_t* hash, const uint8_t* signature, size_t sigSize);

    // eFuse operations
    bool writeToEfuse(const uint8_t* data, size_t size, esp_efuse_block_t block);
    bool readFromEfuse(uint8_t* buffer, size_t size, esp_efuse_block_t block);

    // Utility functions
    void logSecureBootEvent(const String& event, const String& details = "");
    String getEfuseErrorString(esp_err_t err);
};

#endif // SECURE_BOOT_H