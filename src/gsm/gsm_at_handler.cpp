#include "gsm_at_handler.h"

GSMATHandler::GSMATHandler(HardwareSerial* serial, int rstPin) 
    : gsmSerial(serial), resetPin(rstPin) {
}

GSMATHandler::~GSMATHandler() {
}

bool GSMATHandler::begin(unsigned long baud) {
    if (!gsmSerial) {
        Serial.println("[GSM] Error: Serial not initialized");
        return false;
    }
    
    gsmSerial->begin(baud);
    delay(1000);
    
    // Hardware reset if pin configured
    if (resetPin >= 0) {
        pinMode(resetPin, OUTPUT);
        hardwareReset();
    }
    
    // Wait for module to be ready
    delay(3000);
    
    // Check if module responds
    if (!isResponsive()) {
        Serial.println("[GSM] Module not responding");
        return false;
    }
    
    // Disable echo
    sendATCommand("ATE0");
    
    // Set text mode for SMS
    if (!setTextMode()) {
        Serial.println("[GSM] Failed to set text mode");
        return false;
    }
    
    Serial.println("[GSM] Initialized successfully");
    return true;
}

bool GSMATHandler::reset() {
    Serial.println("[GSM] Resetting module...");
    
    if (resetPin >= 0) {
        hardwareReset();
    } else {
        sendATCommand("AT+CFUN=1,1", "OK", 10000); // Software reset
    }
    
    delay(5000);
    return isResponsive();
}

void GSMATHandler::hardwareReset() {
    digitalWrite(resetPin, LOW);
    delay(100);
    digitalWrite(resetPin, HIGH);
    delay(100);
    digitalWrite(resetPin, LOW);
}

bool GSMATHandler::sendATCommand(const char* cmd, const char* expectedResponse, 
                                 unsigned long timeout) {
    clearBuffer();
    
    gsmSerial->println(cmd);
    Serial.printf("[GSM] TX: %s\n", cmd);
    
    return waitForResponse(expectedResponse, timeout);
}

String GSMATHandler::sendATCommandWithResponse(const char* cmd, unsigned long timeout) {
    clearBuffer();
    
    gsmSerial->println(cmd);
    Serial.printf("[GSM] TX: %s\n", cmd);
    
    return readResponse(timeout);
}

bool GSMATHandler::waitForResponse(const char* expected, unsigned long timeout) {
    String response = readResponse(timeout);
    bool found = response.indexOf(expected) != -1;
    
    if (!found) {
        Serial.printf("[GSM] Expected '%s' not found in response\n", expected);
    }
    
    return found;
}

String GSMATHandler::readResponse(unsigned long timeout) {
    responseBuffer = "";
    unsigned long startTime = millis();
    
    while (millis() - startTime < timeout) {
        if (gsmSerial->available()) {
            char c = gsmSerial->read();
            responseBuffer += c;
            
            // Check for common terminators
            if (responseBuffer.endsWith("OK\r\n") || 
                responseBuffer.endsWith("ERROR\r\n") ||
                responseBuffer.endsWith("> ")) {
                break;
            }
        }
        yield();
    }
    
    Serial.printf("[GSM] RX: %s\n", responseBuffer.c_str());
    return responseBuffer;
}

void GSMATHandler::clearBuffer() {
    while (gsmSerial->available()) {
        gsmSerial->read();
    }
    responseBuffer = "";
}

bool GSMATHandler::isResponsive() {
    for (int i = 0; i < MAX_RETRIES; i++) {
        if (sendATCommand("AT", "OK", 1000)) {
            return true;
        }
        delay(500);
    }
    return false;
}

bool GSMATHandler::checkSIMReady() {
    String response = sendATCommandWithResponse("AT+CPIN?");
    return response.indexOf("+CPIN: READY") != -1;
}

String GSMATHandler::getICCID() {
    String response = sendATCommandWithResponse("AT+CCID");
    
    // Parse ICCID from response
    int start = response.indexOf("+CCID: ");
    if (start != -1) {
        start += 7;
        int end = response.indexOf('\r', start);
        if (end != -1) {
            return response.substring(start, end);
        }
    }
    
    return "";
}

String GSMATHandler::getIMSI() {
    String response = sendATCommandWithResponse("AT+CIMI");
    
    // IMSI is returned directly (15 digits)
    response.trim();
    int start = response.indexOf('\n');
    if (start != -1) {
        start++;
        int end = response.indexOf('\r', start);
        if (end != -1) {
            String imsi = response.substring(start, end);
            imsi.trim();
            if (imsi.length() >= 14 && imsi.length() <= 15) {
                return imsi;
            }
        }
    }
    
    return "";
}

int GSMATHandler::getSignalQuality() {
    String response = sendATCommandWithResponse("AT+CSQ");
    
    // Parse: +CSQ: <rssi>,<ber>
    int start = response.indexOf("+CSQ: ");
    if (start != -1) {
        start += 6;
        int comma = response.indexOf(',', start);
        if (comma != -1) {
            String rssi = response.substring(start, comma);
            return rssi.toInt();
        }
    }
    
    return 99; // Unknown
}

bool GSMATHandler::isNetworkRegistered() {
    String response = sendATCommandWithResponse("AT+CREG?");
    
    // Parse: +CREG: <n>,<stat>
    // stat: 1 = registered home, 5 = registered roaming
    int start = response.indexOf("+CREG: ");
    if (start != -1) {
        // Find second number (registration status)
        int comma = response.indexOf(',', start);
        if (comma != -1) {
            char stat = response.charAt(comma + 1);
            return (stat == '1' || stat == '5');
        }
    }
    
    return false;
}

String GSMATHandler::getOperatorName() {
    String response = sendATCommandWithResponse("AT+COPS?");
    
    // Parse: +COPS: <mode>,<format>,"<operator>"
    int start = response.indexOf('"');
    if (start != -1) {
        start++;
        int end = response.indexOf('"', start);
        if (end != -1) {
            return response.substring(start, end);
        }
    }
    
    return "";
}

bool GSMATHandler::setTextMode() {
    return sendATCommand("AT+CMGF=1"); // 1 = Text mode
}

bool GSMATHandler::setPDUMode() {
    return sendATCommand("AT+CMGF=0"); // 0 = PDU mode
}

bool GSMATHandler::setUCS2Mode() {
    return sendATCommand("AT+CSCS=\"UCS2\"");
}

bool GSMATHandler::sendSMS(const char* phoneNumber, const char* message) {
    // Set text mode
    if (!setTextMode()) {
        return false;
    }
    
    // Set phone number
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+CMGS=\"%s\"", phoneNumber);
    
    clearBuffer();
    gsmSerial->println(cmd);
    
    // Wait for '>' prompt
    if (!waitForResponse(">", 5000)) {
        Serial.println("[GSM] No prompt received");
        return false;
    }
    
    // Send message
    gsmSerial->print(message);
    gsmSerial->write(26); // Ctrl+Z to send
    
    // Wait for confirmation
    return waitForResponse("+CMGS:", SMS_TIMEOUT);
}

bool GSMATHandler::sendSMS_UCS2(const char* phoneNumber, const char* ucs2Message) {
    // Set UCS2 character set
    if (!setUCS2Mode()) {
        return false;
    }
    
    // Set text mode
    if (!setTextMode()) {
        return false;
    }
    
    // Send SMS with UCS2 encoded message
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+CMGS=\"%s\"", phoneNumber);
    
    clearBuffer();
    gsmSerial->println(cmd);
    
    if (!waitForResponse(">", 5000)) {
        return false;
    }
    
    gsmSerial->print(ucs2Message);
    gsmSerial->write(26); // Ctrl+Z
    
    return waitForResponse("+CMGS:", SMS_TIMEOUT);
}

bool GSMATHandler::sendSMS_Persian(const char* phoneNumber, const char* utf8Message) {
    // Convert UTF-8 to UCS2
    String ucs2 = utf8ToUCS2(utf8Message);
    
    if (ucs2.length() == 0) {
        Serial.println("[GSM] Failed to convert to UCS2");
        return false;
    }
    
    return sendSMS_UCS2(phoneNumber, ucs2.c_str());
}

int GSMATHandler::getUnreadSMSCount() {
    String response = sendATCommandWithResponse("AT+CMGL=\"REC UNREAD\"");
    
    // Count occurrences of +CMGL
    int count = 0;
    int pos = 0;
    while ((pos = response.indexOf("+CMGL:", pos)) != -1) {
        count++;
        pos += 6;
    }
    
    return count;
}

String GSMATHandler::readSMS(int index) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CMGR=%d", index);
    
    return sendATCommandWithResponse(cmd);
}

bool GSMATHandler::deleteSMS(int index) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CMGD=%d", index);
    
    return sendATCommand(cmd);
}

bool GSMATHandler::deleteAllSMS() {
    return sendATCommand("AT+CMGD=1,4"); // Delete all SMS
}

void GSMATHandler::printStatus() {
    Serial.println("\n=== GSM Status ===");
    Serial.printf("Responsive: %s\n", isResponsive() ? "YES" : "NO");
    Serial.printf("SIM Ready: %s\n", checkSIMReady() ? "YES" : "NO");
    Serial.printf("ICCID: %s\n", getICCID().c_str());
    Serial.printf("IMSI: %s\n", getIMSI().c_str());
    Serial.printf("Signal: %d\n", getSignalQuality());
    Serial.printf("Network: %s\n", isNetworkRegistered() ? "Registered" : "Not registered");
    Serial.printf("Operator: %s\n", getOperatorName().c_str());
    Serial.println("==================\n");
}

String GSMATHandler::utf8ToUCS2(const char* utf8) {
    // Simplified UTF-8 to UCS2 conversion
    // For production, use proper Unicode library
    
    String result = "";
    size_t len = strlen(utf8);
    
    for (size_t i = 0; i < len; i++) {
        uint16_t codepoint = 0;
        
        if ((utf8[i] & 0x80) == 0) {
            // ASCII (1 byte)
            codepoint = utf8[i];
        } else if ((utf8[i] & 0xE0) == 0xC0) {
            // 2-byte UTF-8
            codepoint = ((utf8[i] & 0x1F) << 6) | (utf8[i+1] & 0x3F);
            i++;
        } else if ((utf8[i] & 0xF0) == 0xE0) {
            // 3-byte UTF-8
            codepoint = ((utf8[i] & 0x0F) << 12) | 
                       ((utf8[i+1] & 0x3F) << 6) | 
                       (utf8[i+2] & 0x3F);
            i += 2;
        }
        
        // Convert to hex string (4 digits)
        char hex[5];
        snprintf(hex, sizeof(hex), "%04X", codepoint);
        result += hex;
    }
    
    return result;
}

String GSMATHandler::hexEncode(const uint16_t* ucs2, size_t length) {
    String result = "";
    
    for (size_t i = 0; i < length; i++) {
        char hex[5];
        snprintf(hex, sizeof(hex), "%04X", ucs2[i]);
        result += hex;
    }
    
    return result;
}
