// Persian SMS Handler Header
#ifndef PERSIAN_SMS_HANDLER_H
#define PERSIAN_SMS_HANDLER_H

#include <Arduino.h>
#include <vector>

class PersianSMSHandler {
public:
    PersianSMSHandler();
    bool begin();

    // Core encoding/decoding functions
    String utf8ToUCS2(const String& utf8Text);
    String ucs2ToUTF8(const String& ucs2Text);

    // PDU message preparation
    String preparePDUMessage(const String& message, const String& recipient);

    // Message splitting for long texts
    std::vector<String> splitLongMessage(const String& message, size_t maxLength = 160);

    // Persian text utilities
    bool isPersianText(const String& text);
    String normalizePersianText(const String& text);

private:
    // UTF-8 encoding/decoding helpers
    uint16_t decodeUTF8(const String& text, size_t& index);
    void encodeUTF8(uint16_t unicode, String& result);

    // PDU formatting helpers
    String formatPhoneNumber(const String& number);
};

#endif // PERSIAN_SMS_HANDLER_H