#ifndef AUTO_SIM_DETECTOR_H
#define AUTO_SIM_DETECTOR_H

#include <Arduino.h>

/**
 * @brief Automatic SIM Card Detection and Management
 * 
 * Detects SIM card insertion/removal in all 20 slots.
 * Reads ICCID/IMSI and performs health checks.
 * 
 * PRD Compliance:
 * - REQ-FW-002: Automatic SIM Detection (<5s detection time)
 * - Supports hot-swapping with debouncing
 * - Health scoring based on signal quality
 */
class AutoSIMDetector {
public:
    AutoSIMDetector();
    ~AutoSIMDetector();
    
    // Initialization
    bool begin();
    
    // Detection
    void scanAllSlots();
    bool isSlotOccupied(int slot);
    bool detectChange(int slot);
    
    // SIM Information
    String getICCID(int slot);
    String getIMSI(int slot);
    int getSignalQuality(int slot);
    
    // Health Scoring
    float getHealthScore(int slot); // 0.0 - 1.0
    void updateHealthMetrics(int slot);
    
    // Status
    int getOccupiedSlotCount();
    void printStatus();
    
    // Configuration
    void setDetectionInterval(unsigned long ms);
    void enableAutoDetection(bool enable);
    
    // Callbacks
    typedef void (*SIMEventCallback)(int slot, bool inserted);
    void onSIMEvent(SIMEventCallback callback);
    
    // Constants
    static const int MAX_SLOTS = 20;
    static const unsigned long DEFAULT_SCAN_INTERVAL = 5000; // 5 seconds
    
private:
    struct SIMSlot {
        bool occupied;
        bool previousState;
        String iccid;
        String imsi;
        int signalQuality;
        float healthScore;
        unsigned long lastSeen;
        int failureCount;
    };
    
    SIMSlot slots[MAX_SLOTS];
    unsigned long scanInterval;
    unsigned long lastScanTime;
    bool autoDetectionEnabled;
    SIMEventCallback eventCallback;
    
    // Detection pins (using GPIO expander or multiplexer)
    const int DETECT_PINS[MAX_SLOTS] = {
        // Define GPIO pins for SIM detection
        // These would be connected to SIM card detect pins
        16, 17, 18, 19, 21, 22, 23, 25,  // Slots 0-7
        26, 27, 32, 33, 34, 35, 36, 39,  // Slots 8-15
        4, 5, 12, 13                      // Slots 16-19
    };
    
    void initializePins();
    void readSIMInfo(int slot);
    void calculateHealthScore(int slot);
    void triggerCallback(int slot, bool inserted);
};

#endif // AUTO_SIM_DETECTOR_H
