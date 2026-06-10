/*
 * SignalBoard v1.0 - ESP32-C6 Pin Configuration
 * Mappatura pin da C6-SignalBoard.pdf schema
 * 
 * Hardware:
 * - ESP32-C6 microcontroller
 * - PCA9685 PWM driver (I2C address 0x40)
 * - INA219 power monitor (I2C address 0x44)
 * - Up to 5 signal devices (15 PWM channels max)
 */

#pragma once

#include <cstdint>

namespace BoardPins {

    // ===== I2C Configuration =====
    constexpr uint8_t I2C_SDA = 8;      // I2C Data line (GPIO8)
    constexpr uint8_t I2C_SCL = 9;      // I2C Clock line (GPIO9)
    constexpr uint32_t I2C_FREQ = 400000; // Standard I2C frequency: 400 kHz
    
    // ===== PCA9685 I2C Addresses =====
    // Hardware address select pins (A0-A5) on PCA9685
    constexpr uint8_t PCA_ADDR_PRIMARY = 0x40;   // Base address (all address pins = GND)
    constexpr uint8_t PCA_ADDR_SECONDARY = 0x41; // For future expansion
    
    // ===== Status & Control Pins =====
    constexpr uint8_t LED_STATUS = 2;   // GPIO2 - Status LED (blue)
    constexpr uint8_t BTN_RESET = 1;    // GPIO1 - Factory reset button (optional)
    
    // ===== Power Monitoring (INA219) =====
    // I2C address 0x44 (configurable via A0 pin)
    constexpr uint8_t INA219_I2C_ADDR = 0x44;
    
    // ===== PCA9685 PWM Channels =====
    // 16 total channels per PCA9685 (0-15)
    // Mapped for 5 signal devices: each uses 3 channels (Red, Yellow, Green)
    
    // Signal 1: Channels 0-2
    constexpr uint8_t SIG1_RED = 0;
    constexpr uint8_t SIG1_YELLOW = 1;
    constexpr uint8_t SIG1_GREEN = 2;
    
    // Signal 2: Channels 3-5
    constexpr uint8_t SIG2_RED = 3;
    constexpr uint8_t SIG2_YELLOW = 4;
    constexpr uint8_t SIG2_GREEN = 5;
    
    // Signal 3: Channels 6-8
    constexpr uint8_t SIG3_RED = 6;
    constexpr uint8_t SIG3_YELLOW = 7;
    constexpr uint8_t SIG3_GREEN = 8;
    
    // Signal 4: Channels 9-11
    constexpr uint8_t SIG4_RED = 9;
    constexpr uint8_t SIG4_YELLOW = 10;
    constexpr uint8_t SIG4_GREEN = 11;
    
    // Signal 5: Channels 12-14 (channel 15 reserved)
    constexpr uint8_t SIG5_RED = 12;
    constexpr uint8_t SIG5_YELLOW = 13;
    constexpr uint8_t SIG5_GREEN = 14;
    
    // ===== PCA9685 PWM Frequency =====
    // Standard servo frequency: 50 Hz (for compatibility)
    // For LED driving: 1000 Hz (less flicker, better response)
    constexpr uint16_t PWM_FREQ_LED = 1000;    // Hz
    constexpr uint16_t PWM_MAX_VALUE = 4095;   // 12-bit PWM resolution
    
    // ===== UART Configuration (Serial Debug) =====
    constexpr uint32_t SERIAL_BAUD = 115200;
    
    // ===== Flash & Storage =====
    // LittleFS configuration (web UI files)
    constexpr const char* LITTLEFS_MOUNT_PATH = "/littlefs";
    constexpr size_t LITTLEFS_SIZE = 1048576; // 1 MB (from platformio.ini)
    
    // ===== NVS (Non-Volatile Storage) Namespaces =====
    // Preserve from legacy architecture for compatibility
    constexpr const char* NVS_NS_NETWORK = "network";
    constexpr const char* NVS_NS_RAILWAY = "railway";
    
} // namespace BoardPins
