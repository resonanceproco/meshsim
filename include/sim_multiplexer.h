// SIM Card Multiplexer Header
#ifndef SIM_MULTIPLEXER_H
#define SIM_MULTIPLEXER_H

#include <Arduino.h>
#include <vector>

class SIMMultiplexer {
public:
    SIMMultiplexer();
    bool begin();
    bool selectSlot(uint8_t slot);
    uint8_t getCurrentSlot();
    bool isSlotPresent(uint8_t slot);
    std::vector<uint8_t> scanPresentSIMs();
    bool resetAllMultiplexers();
    bool testMultiplexer(uint8_t multiplexerIndex);

private:
    uint8_t currentSlot;
    bool initialized;

    void setMultiplexerChannel(uint8_t multiplexer, uint8_t channel);
};

#endif // SIM_MULTIPLEXER_H