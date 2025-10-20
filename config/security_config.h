// Security Configuration
#ifndef SECURITY_CONFIG_H
#define SECURITY_CONFIG_H

#include <Arduino.h>

// Security configuration flags
// NOTE: AES and HMAC keys must NOT be hardcoded. Use `SecureKeyManager` to
// store and rotate keys securely in NVS (encrypted storage) at runtime.

// Key rotation interval (ms)
#define KEY_ROTATION_INTERVAL 86400000  // 24 hours in milliseconds

// Build flags to enable platform-level security features (set via build system)
#define SECURE_BOOT_ENABLED 1
#define FLASH_ENCRYPTION_ENABLED 1

// For key management and runtime retrieval, include and use:
// #include "secure_key_manager.h"
// SecureKeyManager skm; skm.begin(); skm.getAESKey(...);

#endif // SECURITY_CONFIG_H