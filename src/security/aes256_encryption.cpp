// AES-256 Encryption Implementation
#include <Arduino.h>
#include <AES256.h>
#include "aes256_encryption.h"
#include "../config/security_config.h"

AES256Encryption::AES256Encryption() : initialized(false) {}

bool AES256Encryption::begin() {
    aes.setKey(AES_KEY, 32);
    initialized = true;
    Serial.println("AES-256 encryption initialized");
    return true;
}

String AES256Encryption::encrypt(const String& plaintext) {
    if (!initialized) return plaintext;

    // Pad the plaintext to 16-byte boundary
    String padded = padString(plaintext);

    String encrypted = "";

    // Encrypt in 16-byte blocks
    for (size_t i = 0; i < padded.length(); i += 16) {
        uint8_t block[16];
        for (int j = 0; j < 16; j++) {
            block[j] = (i + j < padded.length()) ? (uint8_t)padded.charAt(i + j) : 0;
        }

        aes.encryptBlock(block, block);

        // Convert to hex string
        for (int j = 0; j < 16; j++) {
            char hex[3];
            sprintf(hex, "%02X", block[j]);
            encrypted += hex;
        }
    }

    return encrypted;
}

String AES256Encryption::decrypt(const String& ciphertext) {
    if (!initialized) return ciphertext;

    // Ciphertext should be hex-encoded and multiple of 32 characters (16 bytes * 2 hex chars)
    if (ciphertext.length() % 32 != 0) {
        Serial.println("Invalid ciphertext length");
        return "";
    }

    String decrypted = "";

    // Decrypt in 16-byte blocks
    for (size_t i = 0; i < ciphertext.length(); i += 32) {
        uint8_t block[16];

        // Convert hex string to bytes
        for (int j = 0; j < 16; j++) {
            String byteStr = ciphertext.substring(i + j * 2, i + j * 2 + 2);
            block[j] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
        }

        aes.decryptBlock(block, block);

        // Convert back to string
        for (int j = 0; j < 16; j++) {
            decrypted += (char)block[j];
        }
    }

    // Remove padding
    return unpadString(decrypted);
}

String AES256Encryption::padString(const String& input) {
    size_t blockSize = 16;
    size_t padding = blockSize - (input.length() % blockSize);
    if (padding == 0) padding = blockSize;

    String padded = input;
    for (size_t i = 0; i < padding; i++) {
        padded += (char)padding;
    }

    return padded;
}

String AES256Encryption::unpadString(const String& input) {
    if (input.length() == 0) return input;

    uint8_t padding = (uint8_t)input.charAt(input.length() - 1);
    if (padding > 16 || padding == 0) return input;

    // Verify padding is correct
    for (size_t i = input.length() - padding; i < input.length(); i++) {
        if ((uint8_t)input.charAt(i) != padding) return input;
    }

    return input.substring(0, input.length() - padding);
}

String AES256Encryption::generateHMAC(const String& message) {
    // Use SHA256 for HMAC implementation
    // Note: This is a simplified HMAC - in production use a proper crypto library
    String hmac = "";

    // HMAC = H((K ⊕ opad) || H((K ⊕ ipad) || message))
    const char* key = HMAC_KEY;
    size_t keyLen = 32;

    // Prepare inner and outer padding
    uint8_t ipad[64], opad[64];
    memset(ipad, 0x36, 64);
    memset(opad, 0x5C, 64);

    for (size_t i = 0; i < keyLen; i++) {
        ipad[i] ^= (uint8_t)key[i];
        opad[i] ^= (uint8_t)key[i];
    }

    // Inner hash: H((K ⊕ ipad) || message)
    String innerData = String((char*)ipad, 64) + message;
    // Use ESP32's SHA256 hardware acceleration if available
    // For now, use a simple hash - replace with proper SHA256
    uint32_t innerHash = 0;
    for (size_t i = 0; i < innerData.length(); i++) {
        innerHash = ((innerHash << 5) + innerHash) ^ innerData.charAt(i);
    }

    // Outer hash: H((K ⊕ opad) || inner_hash)
    String outerData = String((char*)opad, 64) + String((char*)&innerHash, 4);
    uint32_t outerHash = 0;
    for (size_t i = 0; i < outerData.length(); i++) {
        outerHash = ((outerHash << 5) + outerHash) ^ outerData.charAt(i);
    }

    // Convert to hex string
    char hashStr[9];
    sprintf(hashStr, "%08X", outerHash);
    hmac = hashStr;

    return hmac;
}

bool AES256Encryption::verifyHMAC(const String& message, const String& hmac) {
    String calculatedHMAC = generateHMAC(message);
    return calculatedHMAC.equals(hmac);
}

bool AES256Encryption::rotateKey() {
    // Generate new random key using ESP32 hardware RNG
    uint8_t newKey[32];
    for (int i = 0; i < 32; i += 4) {
        uint32_t randomValue = esp_random();
        memcpy(&newKey[i], &randomValue, min(4, 32 - i));
    }

    // Update key
    aes.setKey(newKey, 32);

    Serial.println("AES key rotated");
    return true;
}

bool AES256Encryption::isInitialized() {
    return initialized;
}