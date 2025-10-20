#include "../config/security_config.h"
#include "secure_key_manager.h"

/**
 * @file security_config.cpp
 * @brief Security configuration runtime utilities
 * 
 * This file provides runtime security configuration helpers.
 * All cryptographic keys MUST be loaded from SecureKeyManager (NVS storage).
 * 
 * DO NOT add hardcoded keys here!
 */

// Global secure key manager instance
SecureKeyManager* g_keyManager = nullptr;

/**
 * Initialize security system
 * Must be called during setup()
 */
bool initializeSecurity() {
    Serial.println("[Security] Initializing security subsystem...");
    
    g_keyManager = new SecureKeyManager();
    
    if (!g_keyManager) {
        Serial.println("[Security] ERROR: Failed to allocate key manager");
        return false;
    }
    
    if (!g_keyManager->begin()) {
        Serial.println("[Security] ERROR: Key manager initialization failed");
        delete g_keyManager;
        g_keyManager = nullptr;
        return false;
    }
    
    Serial.println("[Security] Security subsystem initialized successfully");
    return true;
}

/**
 * Get global key manager instance
 */
SecureKeyManager* getKeyManager() {
    return g_keyManager;
}

/**
 * Perform key rotation check
 * Call this periodically (e.g., in loop())
 */
void checkKeyRotation() {
    if (g_keyManager && g_keyManager->shouldRotateKeys()) {
        Serial.println("[Security] Performing automatic key rotation...");
        if (g_keyManager->rotateKeys()) {
            Serial.println("[Security] Key rotation completed");
        } else {
            Serial.println("[Security] WARNING: Key rotation failed");
        }
    }
}

/**
 * Print security status for diagnostics
 */
void printSecurityStatus() {
    Serial.println("\n=== Security Status ===");
    Serial.printf("Key manager: %s\n", g_keyManager ? "Initialized" : "NOT initialized");
    
    if (g_keyManager) {
        Serial.printf("Keys initialized: %s\n", 
                     g_keyManager->areKeysInitialized() ? "YES" : "NO");
        
        unsigned long timeSinceRotation = g_keyManager->getTimeSinceLastRotation();
        Serial.printf("Time since last rotation: %lu ms (%.1f hours)\n",
                     timeSinceRotation, timeSinceRotation / 3600000.0);
        
        unsigned long timeUntilRotation = KEY_ROTATION_INTERVAL - timeSinceRotation;
        Serial.printf("Time until next rotation: %lu ms (%.1f hours)\n",
                     timeUntilRotation, timeUntilRotation / 3600000.0);
    }
    
    Serial.printf("Secure boot: %s\n", SECURE_BOOT_ENABLED ? "ENABLED" : "DISABLED");
    Serial.printf("Flash encryption: %s\n", FLASH_ENCRYPTION_ENABLED ? "ENABLED" : "DISABLED");
    Serial.println("=======================\n");
}
