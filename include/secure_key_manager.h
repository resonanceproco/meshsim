#ifndef SECURE_KEY_MANAGER_H
#define SECURE_KEY_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <mbedtls/sha256.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

/**
 * @brief Secure Key Manager for ESP32-S3
 * 
 * Manages AES and HMAC keys using NVS encrypted storage.
 * Implements automatic key rotation every 24 hours.
 * Uses hardware RNG for cryptographically secure key generation.
 * 
 * PRD Compliance:
 * - REQ-FW-101: AES-256 encryption with key rotation
 * - Security: No hardcoded keys
 * - Key rotation: 24h interval as specified
 */
class SecureKeyManager {
public:
    SecureKeyManager();
    ~SecureKeyManager();
    
    // Initialization
    bool begin();
    
    // Key management
    bool getAESKey(uint8_t* key, size_t keyLen);
    bool getHMACKey(uint8_t* key, size_t keyLen);
    bool rotateKeys();
    
    // Key rotation management
    bool shouldRotateKeys();
    unsigned long getTimeSinceLastRotation();
    
    // Key generation (for initial setup or rotation)
    bool generateSecureKey(uint8_t* key, size_t keyLen);
    
    // Check if keys are initialized
    bool areKeysInitialized();
    
private:
    Preferences prefs;
    
    // Key rotation tracking
    unsigned long lastRotationTime;
    const unsigned long ROTATION_INTERVAL = 86400000; // 24 hours in ms
    
    // Key storage keys in NVS
    const char* NVS_NAMESPACE = "secure_keys";
    const char* AES_KEY_NAME = "aes_key";
    const char* HMAC_KEY_NAME = "hmac_key";
    const char* ROTATION_TIME_NAME = "last_rotation";
    
    // Hardware RNG for secure key generation
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    
    bool initializeRNG();
    bool storeKey(const char* keyName, const uint8_t* key, size_t keyLen);
    bool loadKey(const char* keyName, uint8_t* key, size_t keyLen);
};

#endif // SECURE_KEY_MANAGER_H
