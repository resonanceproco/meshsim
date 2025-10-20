#ifndef GSM_AT_HANDLER_H
#define GSM_AT_HANDLER_H

#include <Arduino.h>
#include <HardwareSerial.h>

/**
 * @brief GSM AT Command Handler for SIM800C/SIM7600
 * 
 * Handles AT command communication with GSM module for SMS operations.
 * Implements retry logic, timeout handling, and UCS2 encoding support.
 * 
 * PRD Compliance:
 * - REQ-FW-003: Persian SMS Support via UCS2 encoding
 * - Performance: SMS send time < 8s (p95)
 * 
 * Supported Operations:
 * - Send SMS (text/UCS2)
 * - Read SMS
 * - Check signal quality
 * - Get SIM status (ICCID, IMSI)
 * - Network registration
 */
class GSMATHandler {
public:
    GSMATHandler(HardwareSerial* serial, int resetPin = -1);
    ~GSMATHandler();
    
    // Initialization
    bool begin(unsigned long baud = 115200);
    bool reset();
    
    // Basic AT commands
    bool sendATCommand(const char* cmd, const char* expectedResponse = "OK", 
                       unsigned long timeout = 1000);
    String sendATCommandWithResponse(const char* cmd, unsigned long timeout = 1000);
    
    // SIM operations
    bool checkSIMReady();
    String getICCID();
    String getIMSI();
    int getSignalQuality(); // Returns RSSI (0-31, 99=unknown)
    
    // Network operations
    bool isNetworkRegistered();
    String getOperatorName();
    
    // SMS operations
    bool sendSMS(const char* phoneNumber, const char* message);
    bool sendSMS_UCS2(const char* phoneNumber, const char* ucs2Message);
    bool sendSMS_Persian(const char* phoneNumber, const char* utf8Message);
    
    int getUnreadSMSCount();
    String readSMS(int index);
    bool deleteSMS(int index);
    bool deleteAllSMS();
    
    // Configuration
    bool setTextMode(); // Set SMS to text mode
    bool setPDUMode();  // Set SMS to PDU mode
    bool setUCS2Mode(); // Set character set to UCS2
    
    // Diagnostics
    void printStatus();
    bool isResponsive();
    
    // Constants
    static const int MAX_RETRIES = 3;
    static const unsigned long DEFAULT_TIMEOUT = 5000;
    static const unsigned long SMS_TIMEOUT = 30000;
    
private:
    HardwareSerial* gsmSerial;
    int resetPin;
    
    String responseBuffer;
    
    // Helper methods
    bool waitForResponse(const char* expected, unsigned long timeout);
    String readResponse(unsigned long timeout);
    void clearBuffer();
    void hardwareReset();
    
    // UCS2 encoding
    String utf8ToUCS2(const char* utf8);
    String hexEncode(const uint16_t* ucs2, size_t length);
};

#endif // GSM_AT_HANDLER_H
