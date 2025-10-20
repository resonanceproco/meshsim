#include "secure_key_manager.h"

SecureKeyManager::SecureKeyManager() : lastRotationTime(0) {
}

SecureKeyManager::~SecureKeyManager() {
    prefs.end();
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
}

bool SecureKeyManager::begin() {
    // Initialize NVS with encryption
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        Serial.println("[SecureKeyManager] Failed to initialize NVS");
        return false;
    }
    
    // Initialize hardware RNG
    if (!initializeRNG()) {
        Serial.println("[SecureKeyManager] Failed to initialize RNG");
        return false;
    }
    
    // Load last rotation time
    lastRotationTime = prefs.getULong(ROTATION_TIME_NAME, 0);
    
    // Check if keys exist, if not generate them
    if (!areKeysInitialized()) {
        Serial.println("[SecureKeyManager] Keys not found, generating new keys...");
        if (!rotateKeys()) {
            Serial.println("[SecureKeyManager] Failed to generate initial keys");
            return false;
        }
        Serial.println("[SecureKeyManager] Initial keys generated successfully");
    } else {
        Serial.println("[SecureKeyManager] Keys loaded from NVS");
    }
    
    return true;
}

bool SecureKeyManager::initializeRNG() {
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    
    const char* personalization = "ESP32-S3-SIM-MESH";
    int ret = mbedtls_ctr_drbg_seed(
        &ctr_drbg,
        mbedtls_entropy_func,
        &entropy,
        (const unsigned char*)personalization,
        strlen(personalization)
    );
    
    if (ret != 0) {
        Serial.printf("[SecureKeyManager] RNG seed failed: -0x%04x\n", -ret);
        return false;
    }
    
    return true;
}

bool SecureKeyManager::getAESKey(uint8_t* key, size_t keyLen) {
    if (keyLen != 32) {
        Serial.println("[SecureKeyManager] Invalid AES key length (expected 32 bytes)");
        return false;
    }
    return loadKey(AES_KEY_NAME, key, keyLen);
}

bool SecureKeyManager::getHMACKey(uint8_t* key, size_t keyLen) {
    if (keyLen != 32) {
        Serial.println("[SecureKeyManager] Invalid HMAC key length (expected 32 bytes)");
        return false;
    }
    return loadKey(HMAC_KEY_NAME, key, keyLen);
}

bool SecureKeyManager::generateSecureKey(uint8_t* key, size_t keyLen) {
    int ret = mbedtls_ctr_drbg_random(&ctr_drbg, key, keyLen);
    if (ret != 0) {
        Serial.printf("[SecureKeyManager] Key generation failed: -0x%04x\n", -ret);
        return false;
    }
    return true;
}

bool SecureKeyManager::rotateKeys() {
    Serial.println("[SecureKeyManager] Starting key rotation...");
    
    // Generate new AES key
    uint8_t newAESKey[32];
    if (!generateSecureKey(newAESKey, 32)) {
        Serial.println("[SecureKeyManager] Failed to generate new AES key");
        return false;
    }
    
    // Generate new HMAC key
    uint8_t newHMACKey[32];
    if (!generateSecureKey(newHMACKey, 32)) {
        Serial.println("[SecureKeyManager] Failed to generate new HMAC key");
        return false;
    }
    
    // Store new keys in NVS
    if (!storeKey(AES_KEY_NAME, newAESKey, 32)) {
        Serial.println("[SecureKeyManager] Failed to store new AES key");
        return false;
    }
    
    if (!storeKey(HMAC_KEY_NAME, newHMACKey, 32)) {
        Serial.println("[SecureKeyManager] Failed to store new HMAC key");
        return false;
    }
    
    // Update rotation timestamp
    lastRotationTime = millis();
    if (!prefs.putULong(ROTATION_TIME_NAME, lastRotationTime)) {
        Serial.println("[SecureKeyManager] Failed to store rotation time");
        return false;
    }
    
    Serial.println("[SecureKeyManager] Key rotation completed successfully");
    
    // Clear sensitive data from stack
    memset(newAESKey, 0, sizeof(newAESKey));
    memset(newHMACKey, 0, sizeof(newHMACKey));
    
    return true;
}

bool SecureKeyManager::shouldRotateKeys() {
    if (lastRotationTime == 0) {
        return true; // Force rotation if never rotated
    }
    
    unsigned long elapsed = millis() - lastRotationTime;
    
    // Handle millis() overflow
    if (elapsed > ROTATION_INTERVAL * 2) {
        return true;
    }
    
    return elapsed >= ROTATION_INTERVAL;
}

unsigned long SecureKeyManager::getTimeSinceLastRotation() {
    if (lastRotationTime == 0) {
        return 0;
    }
    return millis() - lastRotationTime;
}

bool SecureKeyManager::areKeysInitialized() {
    // Check if both keys exist in NVS
    size_t aesLen = prefs.getBytesLength(AES_KEY_NAME);
    size_t hmacLen = prefs.getBytesLength(HMAC_KEY_NAME);
    
    return (aesLen == 32 && hmacLen == 32);
}

bool SecureKeyManager::storeKey(const char* keyName, const uint8_t* key, size_t keyLen) {
    size_t written = prefs.putBytes(keyName, key, keyLen);
    if (written != keyLen) {
        Serial.printf("[SecureKeyManager] Failed to store key '%s'\n", keyName);
        return false;
    }
    return true;
}

bool SecureKeyManager::loadKey(const char* keyName, uint8_t* key, size_t keyLen) {
    size_t read = prefs.getBytes(keyName, key, keyLen);
    if (read != keyLen) {
        Serial.printf("[SecureKeyManager] Failed to load key '%s'\n", keyName);
        return false;
    }
    return true;
}
