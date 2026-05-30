#ifndef CONFIG_H
#define CONFIG_H

/*
 * SMART IRRIGATION SYSTEM - CONFIGURATION
 * Customize these values for your specific setup
 */

// ========== PIN DEFINITIONS ==========
// Change these if your wiring differs
#define PIN_PUMP        32  // Pump control MOSFET
#define PIN_MOISTURE    34  // Moisture sensor (ADC only pin)
#define PIN_DHT         27  // DHT11 data pin
#define PIN_LED         23  // Status LED

// Button pins
#define BTN_UP          4
#define BTN_DOWN        2
#define BTN_LEFT        15
#define BTN_RIGHT       13
#define BTN_SELECT      12
#define BTN_BACK        14

// I2C pins for LCD
#define PIN_SDA         21
#define PIN_SCL         22

// ========== LCD CONFIGURATION ==========
#define LCD_ADDRESS     0x27  // Try 0x3F if 0x27 doesn't work
#define LCD_COLS        16
#define LCD_ROWS        2

// ========== SENSOR CALIBRATION ==========
// Adjust these based on YOUR sensor readings
#define MOISTURE_RAW_WET    1200  // Sensor in water
#define MOISTURE_RAW_DRY    2700  // Sensor in air

// ========== SAFETY LIMITS ==========
#define PUMP_MAX_RUNTIME    300000  // 5 minutes (milliseconds)
#define MAX_CYCLE_TIME      300     // 5 minutes per cycle (seconds)
#define MAX_CYCLES          10      // Maximum number of cycles

// ========== TIMING CONSTANTS ==========
#define DEBOUNCE_DELAY      50      // Button debounce (ms)
#define LCD_REFRESH_MS      500     // LCD update rate
#define SENSOR_READ_MS      2000    // Sensor reading interval
#define WEB_UPDATE_MS       1000    // Web update interval

// ========== WIFI CONFIGURATION ==========
// Access Point credentials (default)
#define AP_SSID             "SmartIrrigation"
#define AP_PASSWORD         "12345678"  // Change for security!

// ========== TIMEZONE CONFIGURATION ==========
// Adjust for your location
// Examples:
//   UTC: "UTC0"
//   EST: "EST5EDT,M3.2.0,M11.1.0"
//   PST: "PST8PDT,M3.2.0,M11.1.0"
//   IST: "IST-5:30"
#define TIMEZONE            "UTC0"

// ========== DEFAULT SETTINGS ==========
struct DefaultConfig {
    static constexpr int MOISTURE_DRY = 30;
    static constexpr int MOISTURE_WET = 70;
    static constexpr int CYCLE_ON_TIME = 30;
    static constexpr int CYCLE_SOAK_TIME = 60;
    static constexpr int CYCLE_REPEAT = 3;
    static constexpr int DEFAULT_MODE = 2; // AUTO
};

// ========== FEATURE FLAGS ==========
#define ENABLE_SERIAL_DEBUG     true
#define ENABLE_WEB_SERVER       true
#define ENABLE_LCD_DISPLAY      true
#define ENABLE_FERTILIZER       true
#define ENABLE_STATISTICS       true

#endif // CONFIG_H