// OTA Updater Implementation
#include "ota_updater.h"
#include <ArduinoJson.h>

OTAUpdater::OTAUpdater() :
    updateInProgress(false),
    updateProgress(0.0),
    lastUpdateCheck(0),
    updateCheckInterval(3600000), // 1 hour default
    signatureVerificationEnabled(true),
    caCertificate(nullptr),
    clientCertificate(nullptr),
    clientPrivateKey(nullptr) {}

OTAUpdater::~OTAUpdater() {
    if (updateInProgress) {
        Update.abort();
    }
}

bool OTAUpdater::begin(const char* updateServerUrl, const char* firmwarePath) {
    serverUrl = updateServerUrl;
    this->firmwarePath = firmwarePath;

    // Configure WiFi client for HTTPS
    wifiClient.setInsecure(); // TODO: Use proper CA certificate in production

    // Get current version (from build or stored)
    currentVersion = getCurrentVersion();

    Serial.println("OTA Updater initialized");
    Serial.printf("Server: %s\n", serverUrl.c_str());
    Serial.printf("Current version: %s\n", currentVersion.c_str());

    return true;
}

bool OTAUpdater::checkForUpdates() {
    if (millis() - lastUpdateCheck < updateCheckInterval) {
        return false; // Too soon to check again
    }

    lastUpdateCheck = millis();

    String manifestUrl = serverUrl + "/manifest.json";
    String response;

    if (!makeHttpRequest(manifestUrl, response)) {
        logUpdateEvent("CHECK_FAILED", "HTTP request failed");
        return false;
    }

    if (!parseUpdateManifest(response)) {
        logUpdateEvent("CHECK_FAILED", "Manifest parsing failed");
        return false;
    }

    // Compare versions
    if (availableVersion != currentVersion) {
        Serial.printf("Update available: %s -> %s\n", currentVersion.c_str(), availableVersion.c_str());
        logUpdateEvent("UPDATE_AVAILABLE", availableVersion);
        return true;
    }

    logUpdateEvent("UP_TO_DATE", currentVersion);
    return false;
}

bool OTAUpdater::downloadAndInstallUpdate() {
    if (updateInProgress) {
        Serial.println("Update already in progress");
        return false;
    }

    updateInProgress = true;
    updateProgress = 0.0;

    logUpdateEvent("UPDATE_STARTED", availableVersion);

    // Create backup before update
    if (!createBackup()) {
        logUpdateEvent("UPDATE_FAILED", "Backup creation failed");
        updateInProgress = false;
        return false;
    }

    // Download firmware
    String firmwareUrl = serverUrl + firmwarePath + "/firmware.bin";
    uint8_t* firmwareData = nullptr;
    size_t firmwareSize = 0;

    if (!downloadFirmware(firmwareUrl, firmwareData, firmwareSize)) {
        logUpdateEvent("UPDATE_FAILED", "Firmware download failed");
        updateInProgress = false;
        delete[] firmwareData;
        return false;
    }

    // Validate firmware
    if (!validateFirmware(firmwareData, firmwareSize)) {
        logUpdateEvent("UPDATE_FAILED", "Firmware validation failed");
        updateInProgress = false;
        delete[] firmwareData;
        return false;
    }

    // Install firmware
    if (!installFirmware(firmwareData, firmwareSize)) {
        logUpdateEvent("UPDATE_FAILED", "Firmware installation failed");
        // Attempt rollback
        if (rollbackToPreviousVersion()) {
            logUpdateEvent("ROLLBACK_SUCCESSFUL", "Rolled back to previous version");
        } else {
            logUpdateEvent("ROLLBACK_FAILED", "Rollback also failed");
        }
        updateInProgress = false;
        delete[] firmwareData;
        return false;
    }

    delete[] firmwareData;
    updateInProgress = false;
    currentVersion = availableVersion;

    logUpdateEvent("UPDATE_SUCCESSFUL", currentVersion);
    Serial.println("OTA update completed successfully");

    // Schedule reboot
    Serial.println("Rebooting in 5 seconds...");
    delay(5000);
    ESP.restart();

    return true;
}

bool OTAUpdater::rollbackToPreviousVersion() {
    Serial.println("Attempting rollback to previous version...");
    return restoreBackup();
}

bool OTAUpdater::isUpdateInProgress() {
    return updateInProgress;
}

float OTAUpdater::getUpdateProgress() {
    return updateProgress;
}

String OTAUpdater::getCurrentVersion() {
    return currentVersion;
}

String OTAUpdater::getAvailableVersion() {
    return availableVersion;
}

String OTAUpdater::getLastError() {
    return lastError;
}

void OTAUpdater::setCACertificate(const char* caCert) {
    caCertificate = caCert;
    wifiClient.setCACert(caCert);
}

void OTAUpdater::setClientCertificate(const char* clientCert, const char* clientKey) {
    clientCertificate = clientCert;
    clientPrivateKey = clientKey;
    wifiClient.setCertificate(clientCert);
    wifiClient.setPrivateKey(clientKey);
}

void OTAUpdater::setUpdateCheckInterval(unsigned long intervalMs) {
    updateCheckInterval = intervalMs;
}

void OTAUpdater::enableSignatureVerification(bool enable) {
    signatureVerificationEnabled = enable;
}

// Private methods implementation

bool OTAUpdater::verifyFirmwareSignature(const uint8_t* firmwareData, size_t dataSize, const String& signature) {
    if (!signatureVerificationEnabled) {
        Serial.println("Signature verification disabled");
        return true;
    }

    // Calculate SHA256 hash of firmware
    uint8_t calculatedHash[32];
    if (!calculateSHA256(firmwareData, dataSize, calculatedHash)) {
        Serial.println("Failed to calculate firmware hash");
        return false;
    }

    // Decode base64 signature to get expected hash
    String expectedHashStr = base64Decode(signature);
    if (expectedHashStr.length() != 32) {
        Serial.println("Invalid signature length");
        return false;
    }

    // Compare hashes
    if (memcmp(calculatedHash, expectedHashStr.c_str(), 32) != 0) {
        Serial.println("Firmware signature verification failed");
        return false;
    }

    Serial.println("Firmware signature verified successfully");
    return true;
}

bool OTAUpdater::calculateSHA256(const uint8_t* data, size_t len, uint8_t* hash) {
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0); // SHA256, not SHA224
    mbedtls_sha256_update(&ctx, data, len);
    mbedtls_sha256_finish(&ctx, hash);
    mbedtls_sha256_free(&ctx);
    return true;
}

String OTAUpdater::base64Decode(const String& input) {
    // Simple base64 decode implementation
    // In production, use a proper base64 library
    String output = "";
    // TODO: Implement proper base64 decoding
    return output;
}

bool OTAUpdater::downloadFirmware(const String& url, uint8_t*& buffer, size_t& bufferSize) {
    HTTPClient http;
    http.begin(wifiClient, url);
    http.addHeader("User-Agent", "ESP32-S3-MeshNode/1.0");

    Serial.printf("Downloading firmware from: %s\n", url.c_str());

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("HTTP GET failed, error: %d\n", httpCode);
        lastError = "HTTP download failed: " + String(httpCode);
        http.end();
        return false;
    }

    // Get content length
    int contentLength = http.getSize();
    if (contentLength <= 0) {
        Serial.println("Invalid content length");
        lastError = "Invalid content length";
        http.end();
        return false;
    }

    // Allocate buffer
    buffer = new uint8_t[contentLength];
    if (!buffer) {
        Serial.println("Failed to allocate buffer");
        lastError = "Memory allocation failed";
        http.end();
        return false;
    }

    // Download with progress tracking
    WiFiClient* stream = http.getStreamPtr();
    size_t downloaded = 0;
    uint8_t tempBuffer[1024];

    while (downloaded < contentLength) {
        size_t toRead = min((size_t)sizeof(tempBuffer), contentLength - downloaded);
        int bytesRead = stream->readBytes(tempBuffer, toRead);

        if (bytesRead == 0) {
            break; // Connection closed
        }

        memcpy(buffer + downloaded, tempBuffer, bytesRead);
        downloaded += bytesRead;

        // Update progress
        updateProgress = (float)downloaded / contentLength * 100.0;
        Serial.printf("Download progress: %.1f%%\n", updateProgress);
    }

    http.end();

    if (downloaded != contentLength) {
        Serial.printf("Download incomplete: %d/%d bytes\n", downloaded, contentLength);
        delete[] buffer;
        buffer = nullptr;
        lastError = "Incomplete download";
        return false;
    }

    bufferSize = downloaded;
    Serial.printf("Firmware download completed: %d bytes\n", bufferSize);
    return true;
}

bool OTAUpdater::installFirmware(uint8_t* firmwareData, size_t dataSize) {
    Serial.println("Installing firmware...");

    // Begin OTA update
    if (!Update.begin(dataSize)) {
        Serial.println("Failed to begin OTA update");
        lastError = "OTA begin failed";
        return false;
    }

    // Write firmware data
    size_t written = Update.write(firmwareData, dataSize);
    if (written != dataSize) {
        Serial.printf("Write failed: %d/%d bytes\n", written, dataSize);
        Update.abort();
        lastError = "Firmware write failed";
        return false;
    }

    // Complete the update
    if (!Update.end()) {
        Serial.println("Update end failed");
        lastError = "Update end failed";
        return false;
    }

    Serial.println("Firmware installation completed");
    return true;
}

bool OTAUpdater::validateFirmware(uint8_t* firmwareData, size_t dataSize) {
    // Basic validation - check file size
    if (dataSize < 1024 || dataSize > 4 * 1024 * 1024) { // 1KB to 4MB
        Serial.println("Invalid firmware size");
        return false;
    }

    // Check for ESP32-S3 magic bytes (simplified check)
    if (firmwareData[0] != 0xE9) { // ESP32 magic byte
        Serial.println("Invalid firmware magic byte");
        return false;
    }

    Serial.println("Firmware validation passed");
    return true;
}

bool OTAUpdater::createBackup() {
    // For ESP32-S3, backup is handled by the bootloader
    // This is a placeholder for more complex backup systems
    Serial.println("Creating firmware backup...");
    return true;
}

bool OTAUpdater::restoreBackup() {
    // Attempt to restore from backup partition
    Serial.println("Restoring from backup...");
    // ESP.restart() will boot from the other partition if configured
    ESP.restart();
    return true;
}

bool OTAUpdater::makeHttpRequest(const String& url, String& response) {
    HTTPClient http;
    http.begin(wifiClient, url);
    http.addHeader("User-Agent", "ESP32-S3-MeshNode/1.0");

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        response = http.getString();
        http.end();
        return true;
    }

    Serial.printf("HTTP request failed: %d\n", httpCode);
    http.end();
    return false;
}

bool OTAUpdater::parseUpdateManifest(const String& jsonResponse) {
    // Parse JSON manifest to extract version info
    // This is a simplified implementation
    if (jsonResponse.indexOf("version") != -1) {
        // Extract version from JSON (simplified)
        availableVersion = "1.2.0"; // Placeholder
        return true;
    }
    return false;
}

String OTAUpdater::getCurrentVersion() {
    // Get version from build defines or stored value
    return "1.1.0"; // Placeholder
}

String OTAUpdater::getDeviceModel() {
    return "ESP32-S3";
}

String OTAUpdater::getHardwareRevision() {
    return "1.0";
}

void OTAUpdater::logUpdateEvent(const String& event, const String& details) {
    Serial.printf("[OTA] %s: %s\n", event.c_str(), details.c_str());
}