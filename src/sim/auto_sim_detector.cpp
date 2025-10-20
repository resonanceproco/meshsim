// Auto SIM Detector - Automatic SIM card detection and inventory
#include <Arduino.h>
#include "auto_sim_detector.h"
#include "sim_multiplexer.h"
#include "../config/sensor_config.h"

#define SIM_DETECTION_TIMEOUT 5000  // 5 seconds
#define SIM_READ_ATTEMPTS 3         // Retry attempts for SIM reading

AutoSIMDetector::AutoSIMDetector() : multiplexer(nullptr), lastScanTime(0) {}

bool AutoSIMDetector::begin(SIMMultiplexer* mux) {
    multiplexer = mux;
    if (!multiplexer) {
        Serial.println("ERROR: SIM multiplexer not provided");
        return false;
    }

    Serial.println("Auto SIM detector initialized");
    return true;
}

std::vector<SIMInfo> AutoSIMDetector::scanAndDetectSIMs() {
    std::vector<SIMInfo> detectedSIMs;

    if (!multiplexer) return detectedSIMs;

    Serial.println("Starting SIM scan...");

    for (uint8_t slot = 0; slot < 20; slot++) {
        if (multiplexer->isSlotPresent(slot)) {
            Serial.printf("SIM detected in slot %d\n", slot);

            // Select the slot
            if (multiplexer->selectSlot(slot)) {
                // Attempt to read SIM information
                SIMInfo simInfo = readSIMInfo(slot);
                if (simInfo.isValid) {
                    detectedSIMs.push_back(simInfo);
                    Serial.printf("SIM in slot %d: ICCID=%s, IMSI=%s\n",
                                slot, simInfo.iccid.c_str(), simInfo.imsi.c_str());
                } else {
                    Serial.printf("Failed to read SIM in slot %d\n", slot);
                }
            }
        }
    }

    lastScanTime = millis();
    Serial.printf("SIM scan complete. Found %d valid SIMs\n", detectedSIMs.size());

    return detectedSIMs;
}

SIMInfo AutoSIMDetector::readSIMInfo(uint8_t slot) {
    SIMInfo info;
    info.slot = slot;
    info.isValid = false;

    // Wait for SIM to be ready
    delay(100);

    // Send AT command to check SIM status
    if (!sendATCommand("AT", 1000)) {
        return info;
    }

    // Get ICCID
    String iccidResponse = sendATCommand("AT+CCID", 2000);
    if (iccidResponse.indexOf("+CCID:") != -1) {
        int start = iccidResponse.indexOf("+CCID:") + 7;
        int end = iccidResponse.indexOf("\r", start);
        if (end == -1) end = iccidResponse.length();
        info.iccid = iccidResponse.substring(start, end);
        info.iccid.trim();
    }

    // Get IMSI
    String imsiResponse = sendATCommand("AT+CIMI", 2000);
    if (imsiResponse.length() > 0 && imsiResponse[0] >= '0' && imsiResponse[0] <= '9') {
        info.imsi = imsiResponse;
        info.imsi.trim();
    }

    // Get phone number if available
    String numberResponse = sendATCommand("AT+CNUM", 2000);
    if (numberResponse.indexOf("+CNUM:") != -1) {
        // Parse phone number from response
        int start = numberResponse.indexOf("\"", numberResponse.indexOf("+CNUM:"));
        if (start != -1) {
            start++; // Skip the quote
            int end = numberResponse.indexOf("\"", start);
            if (end != -1) {
                info.phoneNumber = numberResponse.substring(start, end);
            }
        }
    }

    // Get network information
    String networkResponse = sendATCommand("AT+COPS?", 2000);
    if (networkResponse.indexOf("+COPS:") != -1) {
        // Parse operator information
        info.carrier = parseCarrierFromCOPS(networkResponse);
    }

    // Check signal quality
    String signalResponse = sendATCommand("AT+CSQ", 2000);
    if (signalResponse.indexOf("+CSQ:") != -1) {
        int start = signalResponse.indexOf("+CSQ:") + 6;
        int comma = signalResponse.indexOf(",", start);
        if (comma != -1) {
            String rssiStr = signalResponse.substring(start, comma);
            info.signalStrength = rssiStr.toInt();
        }
    }

    // Mark as valid if we have at least ICCID or IMSI
    info.isValid = (info.iccid.length() > 0 || info.imsi.length() > 0);

    // Calculate health score based on signal strength
    info.healthScore = calculateHealthScore(info.signalStrength);

    return info;
}

String AutoSIMDetector::sendATCommand(const String& command, uint32_t timeout) {
    // This is a placeholder - in real implementation, this would communicate with SIM800C
    // For now, return mock responses based on command

    if (command == "AT") {
        return "OK";
    } else if (command == "AT+CCID") {
        return "+CCID: 8901234567890123456\r\nOK";
    } else if (command == "AT+CIMI") {
        return "432112345678901\r\nOK";
    } else if (command == "AT+CNUM") {
        return "+CNUM: \"\",\"+989123456789\",129\r\nOK";
    } else if (command == "AT+COPS?") {
        return "+COPS: 0,0,\"MCI\",7\r\nOK";
    } else if (command == "AT+CSQ") {
        return "+CSQ: 25,99\r\nOK";
    }

    return "ERROR";
}

String AutoSIMDetector::parseCarrierFromCOPS(const String& copsResponse) {
    // Parse carrier name from +COPS response
    int firstQuote = copsResponse.indexOf("\"");
    if (firstQuote == -1) return "Unknown";

    int secondQuote = copsResponse.indexOf("\"", firstQuote + 1);
    if (secondQuote == -1) return "Unknown";

    return copsResponse.substring(firstQuote + 1, secondQuote);
}

float AutoSIMDetector::calculateHealthScore(int signalStrength) {
    // Calculate health score based on signal strength
    // 0-31 scale from AT+CSQ command
    if (signalStrength >= SIGNAL_QUALITY_EXCELLENT) return 1.0; // 25+
    if (signalStrength >= SIGNAL_QUALITY_GOOD) return 0.8;      // 15-24
    if (signalStrength >= SIGNAL_QUALITY_FAIR) return 0.6;      // 10-14
    if (signalStrength >= SIGNAL_QUALITY_POOR) return 0.4;      // 5-9
    return 0.2; // < 5 or no signal
}

bool AutoSIMDetector::validateSIMInfo(const SIMInfo& info) {
    // Basic validation
    if (!info.isValid) return false;

    // Check ICCID format (19-20 digits)
    if (info.iccid.length() > 0 && (info.iccid.length() < 19 || info.iccid.length() > 20)) {
        return false;
    }

    // Check IMSI format (14-15 digits typically)
    if (info.imsi.length() > 0 && (info.imsi.length() < 14 || info.imsi.length() > 16)) {
        return false;
    }

    return true;
}

unsigned long AutoSIMDetector::getLastScanTime() {
    return lastScanTime;
}