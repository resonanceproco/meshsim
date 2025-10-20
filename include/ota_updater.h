// OTA Updater Header
#ifndef OTA_UPDATER_H
#define OTA_UPDATER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include <mbedtls/sha256.h>

/**
 * @brief Over-The-Air (OTA) Update Manager for ESP32-S3
 *
 * Implements secure firmware updates with:
 * - HTTPS-based update downloads
 * - Firmware signature verification
 * - Incremental update support
 * - Rollback capability
 * - Update progress monitoring
 *
 * PRD Compliance:
 * - REQ-FW-301: Secure OTA updates via HTTPS
 * - REQ-FW-302: Firmware signature verification
 * - REQ-FW-303: Automatic rollback on failed updates
 * - REQ-FW-304: Update progress reporting
 */
class OTAUpdater {
public:
    OTAUpdater();
    ~OTAUpdater();

    // Initialization
    bool begin(const char* updateServerUrl, const char* firmwarePath = "/firmware");

    // Update management
    bool checkForUpdates();
    bool downloadAndInstallUpdate();
    bool rollbackToPreviousVersion();

    // Status and monitoring
    bool isUpdateInProgress();
    float getUpdateProgress();
    String getCurrentVersion();
    String getAvailableVersion();
    String getLastError();

    // Configuration
    void setCACertificate(const char* caCert);
    void setClientCertificate(const char* clientCert, const char* clientKey);
    void setUpdateCheckInterval(unsigned long intervalMs);
    void enableSignatureVerification(bool enable);

private:
    String serverUrl;
    String firmwarePath;
    String currentVersion;
    String availableVersion;
    String lastError;

    WiFiClientSecure wifiClient;
    HTTPClient httpClient;

    bool updateInProgress;
    float updateProgress;
    unsigned long lastUpdateCheck;
    unsigned long updateCheckInterval;

    bool signatureVerificationEnabled;
    const char* caCertificate;
    const char* clientCertificate;
    const char* clientPrivateKey;

    // Firmware signature verification
    bool verifyFirmwareSignature(const uint8_t* firmwareData, size_t dataSize, const String& signature);
    bool calculateSHA256(const uint8_t* data, size_t len, uint8_t* hash);
    String base64Decode(const String& input);

    // Update process
    bool downloadFirmware(const String& url, uint8_t*& buffer, size_t& bufferSize);
    bool installFirmware(uint8_t* firmwareData, size_t dataSize);
    bool validateFirmware(uint8_t* firmwareData, size_t dataSize);

    // Backup and rollback
    bool createBackup();
    bool restoreBackup();

    // HTTP helpers
    bool makeHttpRequest(const String& url, String& response);
    bool parseUpdateManifest(const String& jsonResponse);

    // Utility functions
    String getDeviceModel();
    String getHardwareRevision();
    void logUpdateEvent(const String& event, const String& details = "");
};

#endif // OTA_UPDATER_H