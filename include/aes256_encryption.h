// AES-256 Encryption Header
#ifndef AES256_ENCRYPTION_H
#define AES256_ENCRYPTION_H

#include <Arduino.h>
#include <AES256.h>

class AES256Encryption {
public:
    AES256Encryption();
    bool begin();

    // Core encryption/decryption
    String encrypt(const String& plaintext);
    String decrypt(const String& ciphertext);

    // HMAC for integrity
    String generateHMAC(const String& message);
    bool verifyHMAC(const String& message, const String& hmac);

    // Key management
    bool rotateKey();
    bool isInitialized();

private:
    AES256 aes;
    bool initialized;

    // Padding helpers
    String padString(const String& input);
    String unpadString(const String& input);
};

#endif // AES256_ENCRYPTION_H