#ifndef HMAC_HANDLER_H
#define HMAC_HANDLER_H

#include <Arduino.h>
#include <mbedtls/md.h>
#include <mbedtls/sha256.h>
#include "secure_key_manager.h"

/**
 * @brief HMAC-SHA256 Message Authentication Handler
 * 
 * Provides message integrity and authentication for mesh network communications.
 * Uses HMAC-SHA256 for all mesh messages to prevent tampering.
 * 
 * PRD Compliance:
 * - REQ-FW-101: HMAC-SHA256 for message integrity
 * - Security: Prevents message tampering and replay attacks
 * 
 * Message Format:
 * [Payload (variable)] + [HMAC (32 bytes)] + [Timestamp (4 bytes)] + [Nonce (4 bytes)]
 */
class HMACHandler {
public:
    HMACHandler(SecureKeyManager* keyManager);
    ~HMACHandler();
    
    // Initialization
    bool begin();
    
    // Message signing
    bool signMessage(const uint8_t* message, size_t messageLen, 
                    uint8_t* signature, size_t* signatureLen);
    
    // Message verification
    bool verifyMessage(const uint8_t* message, size_t messageLen,
                      const uint8_t* signature, size_t signatureLen);
    
    // Combined operations (message + HMAC)
    bool appendHMAC(const uint8_t* message, size_t messageLen,
                   uint8_t* output, size_t* outputLen);
    
    bool verifyAndExtract(const uint8_t* signedMessage, size_t signedLen,
                         uint8_t* message, size_t* messageLen);
    
    // Replay attack prevention
    bool isReplayAttack(uint32_t timestamp, uint32_t nonce);
    void updateReplayCache(uint32_t timestamp, uint32_t nonce);
    
    // Constants
    static const size_t HMAC_SIZE = 32;  // SHA256 output size
    static const size_t TIMESTAMP_SIZE = 4;
    static const size_t NONCE_SIZE = 4;
    static const size_t OVERHEAD = HMAC_SIZE + TIMESTAMP_SIZE + NONCE_SIZE; // 40 bytes
    
private:
    SecureKeyManager* keyManager;
    mbedtls_md_context_t md_ctx;
    uint8_t hmacKey[32];
    
    // Replay attack prevention
    static const size_t REPLAY_CACHE_SIZE = 100;
    struct ReplayCacheEntry {
        uint32_t timestamp;
        uint32_t nonce;
        bool used;
    };
    ReplayCacheEntry replayCache[REPLAY_CACHE_SIZE];
    size_t replayCacheIndex;
    
    // Replay attack time window (5 minutes)
    static const uint32_t REPLAY_WINDOW_MS = 300000;
    
    bool computeHMAC(const uint8_t* data, size_t dataLen,
                    const uint8_t* key, size_t keyLen,
                    uint8_t* hmac, size_t* hmacLen);
    
    uint32_t getCurrentTimestamp();
    uint32_t generateNonce();
};

#endif // HMAC_HANDLER_H
