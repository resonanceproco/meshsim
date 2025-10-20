// Sensor and Calibration Configuration
#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

// SIM Slot Detection Thresholds
#define SIM_DETECTION_VOLTAGE_MIN 2.5  // Minimum voltage for SIM detection
#define SIM_DETECTION_VOLTAGE_MAX 3.3  // Maximum voltage for SIM detection

// GSM Signal Quality Thresholds
#define SIGNAL_QUALITY_EXCELLENT 25    // > -75 dBm
#define SIGNAL_QUALITY_GOOD      15    // -75 to -85 dBm
#define SIGNAL_QUALITY_FAIR      10    // -85 to -95 dBm
#define SIGNAL_QUALITY_POOR      5     // -95 to -105 dBm
#define SIGNAL_QUALITY_NONE      0     // < -105 dBm or no signal

// Power Monitoring Calibration
#define VOLTAGE_DIVIDER_RATIO    2.0   // Voltage divider ratio for battery monitoring
#define CURRENT_SENSOR_SENSITIVITY 0.1 // Current sensor sensitivity (V/A)

// Temperature Thresholds (°C)
#define TEMPERATURE_WARNING      60
#define TEMPERATURE_CRITICAL     75
#define TEMPERATURE_SHUTDOWN     85

#endif // SENSOR_CONFIG_H