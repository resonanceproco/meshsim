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

    // Create complete PDU format for SMS submission
    String pdu = "";

    // SMSC address (empty for default)
    pdu += "00";

    // PDU type: SMS-SUBMIT, no reply path, no status report, no validity period
    pdu += "01";

    // Message reference (let network assign)
    pdu += "00";

    // Destination address (recipient)
    String phoneNumber = formatPhoneNumber(recipient);
    int addressLength = phoneNumber.length() / 2;
    char addrLenHex[3];
    sprintf(addrLenHex, "%02X", addressLength);
    pdu += addrLenHex;
    pdu += "91"; // Type-of-address: international
    pdu += phoneNumber;

    // Protocol identifier: default
    pdu += "00";

    // Data coding scheme: UCS2 (16-bit)
    pdu += "08";

    // Validity period: default (maximum)
    pdu += "FF";

    // User data length (number of UCS2 characters)
    int userDataLength = ucs2Message.length() / 2;
    char udLenHex[3];
    sprintf(udLenHex, "%02X", userDataLength);
    pdu += udLenHex;

    // User data: UCS2 encoded message
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
    // SMS max length for UCS2 is 70 characters (140 bytes)
    size_t currentPos = 0;
    size_t effectiveMaxLength = maxLength > 70 ? 70 : maxLength;

    while (currentPos < message.length()) {
        size_t chunkSize = effectiveMaxLength;
        if (currentPos + chunkSize > message.length()) {
            chunkSize = message.length() - currentPos;
        }

        // For Persian text, try to break at word boundaries
        // Look for Persian word separators: space, Persian comma, etc.
        if (currentPos + chunkSize < message.length()) {
            size_t lastSpace = message.lastIndexOf(' ', currentPos + chunkSize);
            size_t lastComma = message.lastIndexOf('،', currentPos + chunkSize);
            size_t lastPeriod = message.lastIndexOf('.', currentPos + chunkSize);

            // Find the best break point
            size_t breakPoint = currentPos + chunkSize;
            if (lastSpace > currentPos && lastSpace > currentPos + chunkSize / 2) {
                breakPoint = lastSpace;
            } else if (lastComma > currentPos && lastComma > currentPos + chunkSize / 2) {
                breakPoint = lastComma;
            } else if (lastPeriod > currentPos && lastPeriod > currentPos + chunkSize / 2) {
                breakPoint = lastPeriod;
            }

            chunkSize = breakPoint - currentPos;

            // Ensure we don't break in the middle of a multi-byte UTF-8 sequence
            while (chunkSize > 0 && (uint8_t)message.charAt(currentPos + chunkSize - 1) >= 0x80) {
                chunkSize--;
            }
        }

        parts.push_back(message.substring(currentPos, currentPos + chunkSize));
        currentPos += chunkSize;

        // Skip any leading whitespace in next part
        while (currentPos < message.length() && message.charAt(currentPos) == ' ') {
            currentPos++;
        }
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
    // Comprehensive Persian text normalization
    String normalized = text;

    // Replace Arabic characters with Persian equivalents
    normalized.replace("ي", "ی");     // Arabic Yeh -> Persian Yeh
    normalized.replace("ك", "ک");     // Arabic Kaf -> Persian Kaf
    normalized.replace("ة", "ه");     // Arabic Teh Marbuta -> Persian Heh
    normalized.replace("ۀ", "ه");     // Arabic Heh with Yeh -> Persian Heh
    normalized.replace("ى", "ی");     // Arabic Alef Maksura -> Persian Yeh
    normalized.replace("ؤ", "و");     // Arabic Waw with Hamza -> Persian Waw
    normalized.replace("ئ", "ی");     // Arabic Yeh with Hamza -> Persian Yeh
    normalized.replace("ء", "");      // Arabic Hamza -> remove
    normalized.replace("آ", "ا");     // Arabic Alef with Madda -> Persian Alef
    normalized.replace("إ", "ا");     // Arabic Alef with Hamza below -> Persian Alef
    normalized.replace("أ", "ا");     // Arabic Alef with Hamza above -> Persian Alef
    normalized.replace("ٱ", "ا");     // Arabic Alef Wasla -> Persian Alef
    normalized.replace("اً", "ا");     // Arabic Alef with Tanwin -> Persian Alef

    // Normalize Persian specific characters
    normalized.replace("ﷲ", "الله"); // Allah ligature
    normalized.replace("ﷳ", "اکبر"); // Akbar ligature
    normalized.replace("ﷴ", "محمد"); // Muhammad ligature
    normalized.replace("ﷵ", "صلعم"); // Peace be upon him
    normalized.replace("ﷶ", "رسول"); // Rasul ligature
    normalized.replace("ﷷ", "علیه"); // Alayhi ligature
    normalized.replace("ﷸ", "وسلم"); // Peace be upon him
    normalized.replace("ﷹ", "صلی"); // Salla ligature
    normalized.replace("ﷺ", "صلی الله علیه وسلم"); // Complete peace be upon him
    normalized.replace("ﷻ", "جل جلاله"); // Glory be to Him

    // Handle Tatweel (elongation character) - remove excessive ones
    while (normalized.indexOf("ــ") != -1) {
        normalized.replace("ــ", "ـ");
    }

    // Normalize spacing around punctuation
    normalized.replace("  ", " "); // Multiple spaces to single
    normalized.replace(" ،", "،"); // Space before comma
    normalized.replace(" .", "."); // Space before period
    normalized.replace(" :", ":"); // Space before colon
    normalized.replace(" ;", ";"); // Space before semicolon

    return normalized;
}