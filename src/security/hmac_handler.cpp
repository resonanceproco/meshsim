#include "hmac_handler.h"

HMACHandler::HMACHandler(SecureKeyManager* keyMgr) 
    : keyManager(keyMgr), replayCacheIndex(0) {
    memset(hmacKey, 0, sizeof(hmacKey));
    memset(replayCache, 0, sizeof(replayCache));
}

HMACHandler::~HMACHandler() {
    mbedtls_md_free(&md_ctx);
    memset(hmacKey, 0, sizeof(hmacKey)); // Clear sensitive data
}

bool HMACHandler::begin() {
    if (!keyManager) {
        Serial.println("[HMACHandler] KeyManager not initialized");
        return false;
    }
    
    // Load HMAC key from secure storage
    if (!keyManager->getHMACKey(hmacKey, sizeof(hmacKey))) {
        Serial.println("[HMACHandler] Failed to load HMAC key");
        return false;
    }
    
    // Initialize mbedtls MD context
    mbedtls_md_init(&md_ctx);
    
    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (md_info == NULL) {
        Serial.println("[HMACHandler] Failed to get SHA256 MD info");
        return false;
    }
    
    int ret = mbedtls_md_setup(&md_ctx, md_info, 1); // 1 = HMAC mode
    if (ret != 0) {
        Serial.printf("[HMACHandler] MD setup failed: -0x%04x\n", -ret);
        return false;
    }
    
    Serial.println("[HMACHandler] Initialized successfully");
    return true;
}

bool HMACHandler::computeHMAC(const uint8_t* data, size_t dataLen,
                             const uint8_t* key, size_t keyLen,
                             uint8_t* hmac, size_t* hmacLen) {
    if (!data || !key || !hmac || !hmacLen) {
        return false;
    }
    
    int ret = mbedtls_md_hmac_starts(&md_ctx, key, keyLen);
    if (ret != 0) {
        Serial.printf("[HMACHandler] HMAC start failed: -0x%04x\n", -ret);
        return false;
    }
    
    ret = mbedtls_md_hmac_update(&md_ctx, data, dataLen);
    if (ret != 0) {
        Serial.printf("[HMACHandler] HMAC update failed: -0x%04x\n", -ret);
        return false;
    }
    
    ret = mbedtls_md_hmac_finish(&md_ctx, hmac);
    if (ret != 0) {
        Serial.printf("[HMACHandler] HMAC finish failed: -0x%04x\n", -ret);
        return false;
    }
    
    *hmacLen = HMAC_SIZE;
    return true;
}

bool HMACHandler::signMessage(const uint8_t* message, size_t messageLen,
                             uint8_t* signature, size_t* signatureLen) {
    if (!message || !signature || !signatureLen) {
        Serial.println("[HMACHandler] Invalid parameters for signMessage");
        return false;
    }
    
    // Create buffer with message + timestamp + nonce
    uint32_t timestamp = getCurrentTimestamp();
    uint32_t nonce = generateNonce();
    
    size_t totalLen = messageLen + TIMESTAMP_SIZE + NONCE_SIZE;
    uint8_t* buffer = (uint8_t*)malloc(totalLen);
    if (!buffer) {
        Serial.println("[HMACHandler] Memory allocation failed");
        return false;
    }
    
    // Combine message + timestamp + nonce
    memcpy(buffer, message, messageLen);
    memcpy(buffer + messageLen, &timestamp, TIMESTAMP_SIZE);
    memcpy(buffer + messageLen + TIMESTAMP_SIZE, &nonce, NONCE_SIZE);
    
    // Compute HMAC
    bool result = computeHMAC(buffer, totalLen, hmacKey, sizeof(hmacKey), 
                             signature, signatureLen);
    
    free(buffer);
    return result;
}

bool HMACHandler::verifyMessage(const uint8_t* message, size_t messageLen,
                               const uint8_t* signature, size_t signatureLen) {
    if (!message || !signature || signatureLen != HMAC_SIZE) {
        Serial.println("[HMACHandler] Invalid parameters for verifyMessage");
        return false;
    }
    
    // Extract timestamp and nonce from message
    if (messageLen < TIMESTAMP_SIZE + NONCE_SIZE) {
        Serial.println("[HMACHandler] Message too short");
        return false;
    }
    
    uint32_t timestamp, nonce;
    memcpy(&timestamp, message + messageLen - TIMESTAMP_SIZE - NONCE_SIZE, TIMESTAMP_SIZE);
    memcpy(&nonce, message + messageLen - NONCE_SIZE, NONCE_SIZE);
    
    // Check for replay attack
    if (isReplayAttack(timestamp, nonce)) {
        Serial.println("[HMACHandler] Replay attack detected!");
        return false;
    }
    
    // Compute HMAC for verification
    uint8_t computedHMAC[HMAC_SIZE];
    size_t computedLen;
    
    if (!computeHMAC(message, messageLen, hmacKey, sizeof(hmacKey),
                    computedHMAC, &computedLen)) {
        Serial.println("[HMACHandler] Failed to compute verification HMAC");
        return false;
    }
    
    // Constant-time comparison to prevent timing attacks
    int diff = 0;
    for (size_t i = 0; i < HMAC_SIZE; i++) {
        diff |= (signature[i] ^ computedHMAC[i]);
    }
    
    if (diff != 0) {
        Serial.println("[HMACHandler] HMAC verification failed");
        return false;
    }
    
    // Update replay cache
    updateReplayCache(timestamp, nonce);
    
    return true;
}

bool HMACHandler::appendHMAC(const uint8_t* message, size_t messageLen,
                            uint8_t* output, size_t* outputLen) {
    if (!message || !output || !outputLen) {
        return false;
    }
    
    uint32_t timestamp = getCurrentTimestamp();
    uint32_t nonce = generateNonce();
    
    // Copy message
    memcpy(output, message, messageLen);
    
    // Append timestamp and nonce
    memcpy(output + messageLen, &timestamp, TIMESTAMP_SIZE);
    memcpy(output + messageLen + TIMESTAMP_SIZE, &nonce, NONCE_SIZE);
    
    size_t dataLen = messageLen + TIMESTAMP_SIZE + NONCE_SIZE;
    
    // Compute and append HMAC
    uint8_t hmac[HMAC_SIZE];
    size_t hmacLen;
    
    if (!computeHMAC(output, dataLen, hmacKey, sizeof(hmacKey), hmac, &hmacLen)) {
        return false;
    }
    
    memcpy(output + dataLen, hmac, HMAC_SIZE);
    *outputLen = dataLen + HMAC_SIZE;
    
    return true;
}

bool HMACHandler::verifyAndExtract(const uint8_t* signedMessage, size_t signedLen,
                                  uint8_t* message, size_t* messageLen) {
    if (!signedMessage || !message || !messageLen) {
        return false;
    }
    
    if (signedLen < OVERHEAD) {
        Serial.println("[HMACHandler] Signed message too short");
        return false;
    }
    
    // Extract HMAC (last 32 bytes)
    const uint8_t* receivedHMAC = signedMessage + signedLen - HMAC_SIZE;
    
    // Extract timestamp and nonce
    size_t dataLen = signedLen - HMAC_SIZE;
    uint32_t timestamp, nonce;
    memcpy(&timestamp, signedMessage + dataLen - TIMESTAMP_SIZE - NONCE_SIZE, TIMESTAMP_SIZE);
    memcpy(&nonce, signedMessage + dataLen - NONCE_SIZE, NONCE_SIZE);
    
    // Check replay attack
    if (isReplayAttack(timestamp, nonce)) {
        Serial.println("[HMACHandler] Replay attack detected!");
        return false;
    }
    
    // Verify HMAC
    uint8_t computedHMAC[HMAC_SIZE];
    size_t computedLen;
    
    if (!computeHMAC(signedMessage, dataLen, hmacKey, sizeof(hmacKey),
                    computedHMAC, &computedLen)) {
        return false;
    }
    
    // Constant-time comparison
    int diff = 0;
    for (size_t i = 0; i < HMAC_SIZE; i++) {
        diff |= (receivedHMAC[i] ^ computedHMAC[i]);
    }
    
    if (diff != 0) {
        Serial.println("[HMACHandler] HMAC verification failed");
        return false;
    }
    
    // Extract original message (without timestamp, nonce, and HMAC)
    *messageLen = dataLen - TIMESTAMP_SIZE - NONCE_SIZE;
    memcpy(message, signedMessage, *messageLen);
    
    // Update replay cache
    updateReplayCache(timestamp, nonce);
    
    return true;
}

bool HMACHandler::isReplayAttack(uint32_t timestamp, uint32_t nonce) {
    uint32_t currentTime = getCurrentTimestamp();
    
    // Check if timestamp is too old (older than replay window)
    if (currentTime - timestamp > REPLAY_WINDOW_MS) {
        Serial.println("[HMACHandler] Message timestamp too old");
        return true;
    }
    
    // Check if timestamp is in the future (clock skew tolerance: 1 minute)
    if (timestamp > currentTime + 60000) {
        Serial.println("[HMACHandler] Message timestamp in future");
        return true;
    }
    
    // Check replay cache for duplicate timestamp+nonce
    for (size_t i = 0; i < REPLAY_CACHE_SIZE; i++) {
        if (replayCache[i].used &&
            replayCache[i].timestamp == timestamp &&
            replayCache[i].nonce == nonce) {
            return true; // Duplicate found
        }
    }
    
    return false;
}

void HMACHandler::updateReplayCache(uint32_t timestamp, uint32_t nonce) {
    replayCache[replayCacheIndex].timestamp = timestamp;
    replayCache[replayCacheIndex].nonce = nonce;
    replayCache[replayCacheIndex].used = true;
    
    replayCacheIndex = (replayCacheIndex + 1) % REPLAY_CACHE_SIZE;
}

uint32_t HMACHandler::getCurrentTimestamp() {
    return millis();
}

uint32_t HMACHandler::generateNonce() {
    return esp_random();
}
