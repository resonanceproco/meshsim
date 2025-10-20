// Persian SMS Handler - UCS2 encoding and decoding for Persian text
#include <Arduino.h>
#include "persian_sms_handler.h"

PersianSMSHandler::PersianSMSHandler() {}

bool PersianSMSHandler::begin() {
    Serial.println("Persian SMS handler initialized");
    return true;
}

String PersianSMSHandler::utf8ToUCS2(const String& utf8Text) {
    String ucs2Result = "";

    for (size_t i = 0; i < utf8Text.length(); ) {
        uint16_t unicode = decodeUTF8(utf8Text, i);

        // Convert Unicode to UCS2 (big-endian)
        char highByte = (unicode >> 8) & 0xFF;
        char lowByte = unicode & 0xFF;

        ucs2Result += highByte;
        ucs2Result += lowByte;
    }

    return ucs2Result;
}

String PersianSMSHandler::ucs2ToUTF8(const String& ucs2Text) {
    String utf8Result = "";

    // UCS2 is always even length (2 bytes per character)
    for (size_t i = 0; i < ucs2Text.length(); i += 2) {
        if (i + 1 >= ucs2Text.length()) break;

        // Read big-endian UCS2 character
        uint16_t unicode = ((uint8_t)ucs2Text.charAt(i) << 8) | (uint8_t)ucs2Text.charAt(i + 1);

        // Convert to UTF-8
        encodeUTF8(unicode, utf8Result);
    }

    return utf8Result;
}

uint16_t PersianSMSHandler::decodeUTF8(const String& text, size_t& index) {
    if (index >= text.length()) return 0;

    uint8_t firstByte = (uint8_t)text.charAt(index++);

    if (firstByte < 0x80) {
        // 1-byte sequence (ASCII)
        return firstByte;
    } else if ((firstByte & 0xE0) == 0xC0) {
        // 2-byte sequence
        if (index >= text.length()) return 0;
        uint8_t secondByte = (uint8_t)text.charAt(index++);
        return ((firstByte & 0x1F) << 6) | (secondByte & 0x3F);
    } else if ((firstByte & 0xF0) == 0xE0) {
        // 3-byte sequence
        if (index + 1 >= text.length()) return 0;
        uint8_t secondByte = (uint8_t)text.charAt(index++);
        uint8_t thirdByte = (uint8_t)text.charAt(index++);
        return ((firstByte & 0x0F) << 12) | ((secondByte & 0x3F) << 6) | (thirdByte & 0x3F);
    }

    // Invalid UTF-8, return replacement character
    return 0xFFFD;
}

void PersianSMSHandler::encodeUTF8(uint16_t unicode, String& result) {
    if (unicode < 0x80) {
        // 1-byte sequence
        result += (char)unicode;
    } else if (unicode < 0x800) {
        // 2-byte sequence
        result += (char)(0xC0 | (unicode >> 6));
        result += (char)(0x80 | (unicode & 0x3F));
    } else {
        // 3-byte sequence
        result += (char)(0xE0 | (unicode >> 12));
        result += (char)(0x80 | ((unicode >> 6) & 0x3F));
        result += (char)(0x80 | (unicode & 0x3F));
    }
}

String PersianSMSHandler::preparePDUMessage(const String& message, const String& recipient) {
    // Convert UTF-8 message to UCS2
    String ucs2Message = utf8ToUCS2(message);

    // Create PDU format for SMS
    // This is a simplified implementation - real PDU encoding is more complex
    String pdu = "00"; // SMSC length (0 = use default)

    // Message flags (UCS2 encoding)
    pdu += "11"; // First octet: Reply path, User data header, Status report, Validity period

    // Message reference
    pdu += "00";

    // Recipient phone number
    String phoneNumber = formatPhoneNumber(recipient);
    pdu += String(phoneNumber.length() / 2 + 1, HEX); // Length of phone number
    pdu += "91"; // International format
    pdu += phoneNumber;

    // Protocol identifier
    pdu += "00";

    // Data coding scheme (UCS2)
    pdu += "08";

    // Validity period
    pdu += "FF";

    // User data length (in septets for 7-bit, but characters for UCS2)
    int userDataLength = ucs2Message.length() / 2; // Each UCS2 char is 2 bytes
    pdu += String(userDataLength, HEX);

    // User data (UCS2 encoded message)
    for (size_t i = 0; i < ucs2Message.length(); i++) {
        char hex[3];
        sprintf(hex, "%02X", (uint8_t)ucs2Message.charAt(i));
        pdu += hex;
    }

    return pdu;
}

String PersianSMSHandler::formatPhoneNumber(const String& number) {
    // Remove any non-digit characters and ensure international format
    String cleanNumber = "";
    for (size_t i = 0; i < number.length(); i++) {
        char c = number.charAt(i);
        if (c >= '0' && c <= '9') {
            cleanNumber += c;
        }
    }

    // Add country code if not present
    if (cleanNumber.startsWith("0")) {
        cleanNumber = "98" + cleanNumber.substring(1);
    } else if (!cleanNumber.startsWith("98")) {
        cleanNumber = "98" + cleanNumber;
    }

    // Ensure even length for BCD encoding
    if (cleanNumber.length() % 2 != 0) {
        cleanNumber += "F";
    }

    return cleanNumber;
}

std::vector<String> PersianSMSHandler::splitLongMessage(const String& message, size_t maxLength) {
    std::vector<String> parts;

    // For Persian text, we need to be careful about character boundaries
    size_t currentPos = 0;

    while (currentPos < message.length()) {
        size_t chunkSize = min(maxLength, message.length() - currentPos);

        // Try to break at word boundaries if possible
        if (currentPos + chunkSize < message.length()) {
            size_t lastSpace = message.lastIndexOf(' ', currentPos + chunkSize);
            if (lastSpace > currentPos && lastSpace > currentPos + chunkSize / 2) {
                chunkSize = lastSpace - currentPos;
            }
        }

        parts.push_back(message.substring(currentPos, currentPos + chunkSize));
        currentPos += chunkSize;
    }

    return parts;
}

bool PersianSMSHandler::isPersianText(const String& text) {
    // Check if text contains Persian/Arabic characters
    for (size_t i = 0; i < text.length(); ) {
        uint16_t unicode = decodeUTF8(text, i);

        // Persian/Arabic Unicode ranges
        if ((unicode >= 0x0600 && unicode <= 0x06FF) || // Arabic
            (unicode >= 0x0750 && unicode <= 0x077F) || // Arabic Supplement
            (unicode >= 0x08A0 && unicode <= 0x08FF) || // Arabic Extended-A
            (unicode >= 0xFB50 && unicode <= 0xFDFF) || // Arabic Presentation Forms-A
            (unicode >= 0xFE70 && unicode <= 0xFEFF)) { // Arabic Presentation Forms-B
            return true;
        }
    }

    return false;
}

String PersianSMSHandler::normalizePersianText(const String& text) {
    // Basic normalization for Persian text
    // This is a simplified implementation - real normalization would be more comprehensive
    String normalized = text;

    // Replace Arabic Yeh with Persian Yeh
    normalized.replace("ي", "ی");
    normalized.replace("ك", "ک");

    return normalized;
}