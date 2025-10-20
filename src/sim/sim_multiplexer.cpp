// SIM Card Multiplexer - Hardware switching for 20 SIM cards
#include <Arduino.h>
#include "sim_multiplexer.h"
#include "../config/sensor_config.h"

#define SIM_SELECT_PINS {2, 4, 5, 12, 13, 14, 15, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33} // 17 pins for 5 multiplexers
#define SIM_DETECT_PINS {34, 35, 36, 39} // 4 pins for SIM detection (one per multiplexer group)

SIMMultiplexer::SIMMultiplexer() : currentSlot(0), initialized(false) {}

bool SIMMultiplexer::begin() {
    // Initialize select pins as outputs
    int selectPins[] = SIM_SELECT_PINS;
    for (int i = 0; i < 17; i++) {
        pinMode(selectPins[i], OUTPUT);
        digitalWrite(selectPins[i], LOW);
    }

    // Initialize detection pins as inputs with pull-up
    int detectPins[] = SIM_DETECT_PINS;
    for (int i = 0; i < 4; i++) {
        pinMode(detectPins[i], INPUT_PULLUP);
    }

    initialized = true;
    Serial.println("SIM multiplexer initialized");
    return true;
}

bool SIMMultiplexer::selectSlot(uint8_t slot) {
    if (slot >= 20 || !initialized) {
        return false;
    }

    // Each TS3A5017 multiplexer controls 4 SIM slots
    // We need 5 multiplexers for 20 slots total
    uint8_t multiplexerIndex = slot / 4;  // Which multiplexer (0-4)
    uint8_t channelIndex = slot % 4;      // Which channel (0-3)

    // Set multiplexer select pins
    setMultiplexerChannel(multiplexerIndex, channelIndex);

    currentSlot = slot;
    delay(10); // Allow settling time

    return true;
}

void SIMMultiplexer::setMultiplexerChannel(uint8_t multiplexer, uint8_t channel) {
    // Each multiplexer needs 4 select pins (2^4 = 16 channels, but we only use 4)
    // We'll use binary encoding for simplicity
    int selectPins[] = SIM_SELECT_PINS;

    // Calculate pin offset for this multiplexer (4 pins per multiplexer)
    uint8_t pinOffset = multiplexer * 4;

    // Set binary value for channel selection
    for (int i = 0; i < 4; i++) {
        bool bit = (channel >> i) & 1;
        digitalWrite(selectPins[pinOffset + i], bit ? HIGH : LOW);
    }
}

uint8_t SIMMultiplexer::getCurrentSlot() {
    return currentSlot;
}

bool SIMMultiplexer::isSlotPresent(uint8_t slot) {
    if (slot >= 20 || !initialized) {
        return false;
    }

    // Determine which detection pin to check
    uint8_t multiplexerIndex = slot / 4;
    int detectPins[] = SIM_DETECT_PINS;

    if (multiplexerIndex >= 4) {
        return false; // Invalid multiplexer index
    }

    // Read detection pin (LOW = SIM present, HIGH = SIM absent)
    int detectPin = detectPins[multiplexerIndex];
    bool simPresent = (digitalRead(detectPin) == LOW);

    return simPresent;
}

std::vector<uint8_t> SIMMultiplexer::scanPresentSIMs() {
    std::vector<uint8_t> presentSlots;

    for (uint8_t slot = 0; slot < 20; slot++) {
        if (isSlotPresent(slot)) {
            presentSlots.push_back(slot);
        }
    }

    return presentSlots;
}

bool SIMMultiplexer::resetAllMultiplexers() {
    if (!initialized) return false;

    int selectPins[] = SIM_SELECT_PINS;
    for (int i = 0; i < 17; i++) {
        digitalWrite(selectPins[i], LOW);
    }

    currentSlot = 0;
    return true;
}

bool SIMMultiplexer::testMultiplexer(uint8_t multiplexerIndex) {
    if (multiplexerIndex >= 5 || !initialized) {
        return false;
    }

    // Test all channels of the multiplexer
    for (uint8_t channel = 0; channel < 4; channel++) {
        setMultiplexerChannel(multiplexerIndex, channel);
        delay(5);

        // Check if we can read back the setting (basic functionality test)
        // In a real implementation, you might check voltage levels or use test pins
    }

    return true;
}