/*
 * SignalBoard v1.0 - ESP32-C6 Firmware
 * Main entry point: setup() and loop()
 * 
 * Phase 1: Hardware initialization only
 * Phase 2: Add MQTT + Web server logic
 */

#include <Arduino.h>
#include <Wire.h>
#include "../include/Pins.h"
#include "../include/Config.h"
#include "../include/Version.h"
#include "hardware/I2C_HAL.h"
#include "hardware/PCA9685_Driver.h"
#include "hardware/SignalDevice.h"

// Hardware instances
I2C_HAL i2c_bus(BoardPins::I2C_SDA, BoardPins::I2C_SCL);
PCA9685_Driver pca9685(&i2c_bus, BoardPins::PCA_ADDR_PRIMARY);

// Signal devices (up to 5)
SignalDevice* signals[Config::MAX_SIGNALS] = {nullptr};
uint8_t signal_count = 0;

// ============================================================================
// SETUP: Called once on boot
// ============================================================================
void setup() {
    Serial.begin(BoardPins::SERIAL_BAUD);
    delay(100);
    
    // Print boot banner
    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.printf("║ SignalBoard v%s - ESP32-C6 Railway Signal Controller ║\n", 
                  FW_VERSION_STRING);
    Serial.println("╠════════════════════════════════════════════════════════════╣");
    Serial.printf("║ Build: %s %s\n", FW_BUILD_DATE, FW_BUILD_TIME);
    Serial.printf("║ Target: %s (%s)\n", HW_TARGET, HW_VARIANT);
    Serial.println("╚════════════════════════════════════════════════════════════╝");

    // Initialize I2C bus
    Serial.print("[INIT] I2C bus at GPIO");
    Serial.print(BoardPins::I2C_SDA);
    Serial.print("/");
    Serial.print(BoardPins::I2C_SCL);
    Serial.print(" (");
    Serial.print(BoardPins::I2C_FREQ / 1000);
    Serial.println(" kHz)...");
    
    if (!i2c_bus.init(BoardPins::I2C_FREQ)) {
        Serial.println("[ERROR] I2C initialization failed!");
        return;
    }
    Serial.println("[OK] I2C bus initialized");

    // Probe PCA9685
    Serial.print("[PROBE] PCA9685 at 0x");
    Serial.print(BoardPins::PCA_ADDR_PRIMARY, HEX);
    Serial.print("... ");
    
    if (!i2c_bus.probe(BoardPins::PCA_ADDR_PRIMARY)) {
        Serial.println("NOT FOUND!");
        return;
    }
    Serial.println("OK");

    // Initialize PCA9685
    Serial.print("[INIT] PCA9685 driver... ");
    if (!pca9685.init()) {
        Serial.println("FAILED!");
        return;
    }
    Serial.println("OK");

    // Set PCA9685 frequency
    Serial.printf("[CONFIG] PCA9685 frequency: %d Hz\n", Config::PWM_FREQUENCY_HZ);
    pca9685.setFrequency(Config::PWM_FREQUENCY_HZ);

    // Initialize status LED
    if (Config::ENABLE_STATUS_LED) {
        pinMode(BoardPins::LED_STATUS, OUTPUT);
        digitalWrite(BoardPins::LED_STATUS, LOW);
        Serial.printf("[CONFIG] Status LED enabled on GPIO%d\n", BoardPins::LED_STATUS);
    }

    Serial.println();
    Serial.println("[READY] SignalBoard initialization complete");
    Serial.println("Next: Load signal configuration from NVS (Phase 2)");
    Serial.println();
}

// ============================================================================
// LOOP: Main event loop (called repeatedly)
// ============================================================================
void loop() {
    // Phase 1: Just blink LED to show we're alive
    if (Config::ENABLE_STATUS_LED) {
        digitalWrite(BoardPins::LED_STATUS, HIGH);
        delay(100);
        digitalWrite(BoardPins::LED_STATUS, LOW);
        delay(900);
    } else {
        delay(1000);
    }

    // Debug output (if enabled)
    if (Config::ENABLE_DEBUG_SERIAL) {
        static unsigned long last_debug = 0;
        if (millis() - last_debug > 10000) {
            last_debug = millis();
            Serial.printf("[DEBUG] Uptime: %lu ms\n", millis());
            Serial.printf("[DEBUG] Free heap: %u bytes\n", ESP.getFreeHeap());
            Serial.printf("[DEBUG] PCA9685 ready: %s\n", pca9685.isInitialized() ? "yes" : "no");
        }
    }
}

// ============================================================================
// Stub functions for Phase 1 testing
// ============================================================================

/**
 * Test function: Set a specific signal aspect (for web/MQTT debugging)
 * @param signal_index 0-4 (signal device index)
 * @param aspect Aspect value
 */
void testSetSignalAspect(uint8_t signal_index, SignalAspect aspect) {
    if (signal_index >= Config::MAX_SIGNALS || !signals[signal_index]) {
        Serial.printf("[TEST] Invalid signal index: %d\n", signal_index);
        return;
    }
    if (signals[signal_index]->setAspect(aspect)) {
        Serial.printf("[TEST] Signal %d set to aspect %d\n", signal_index, (int)aspect);
    }
}

/**
 * Test function: Set PWM on raw PCA channel (for hardware debugging)
 * @param channel PCA9685 channel (0-15)
 * @param value PWM value (0-4095)
 */
void testSetPWMChannel(uint8_t channel, uint16_t value) {
    if (channel >= PCA9685_Driver::CHANNELS) {
        Serial.printf("[TEST] Invalid channel: %d\n", channel);
        return;
    }
    pca9685.setPWM(channel, value);
    Serial.printf("[TEST] PCA channel %d set to %d\n", channel, value);
}

/**
 * System info helper
 */
void printSystemInfo() {
    Serial.println("\n=== System Information ===");
    Serial.printf("Firmware: %s\n", FW_VERSION_STRING);
    Serial.printf("Hardware: %s (%s)\n", HW_TARGET, HW_VARIANT);
    Serial.printf("Build: %s %s\n", FW_BUILD_DATE, FW_BUILD_TIME);
    Serial.printf("Free Heap: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Uptime: %lu ms\n", millis());
    Serial.printf("PCA9685: %s\n", pca9685.isInitialized() ? "Ready" : "Not Ready");
    Serial.printf("I2C Bus: %s\n", i2c_bus.isInitialized() ? "Ready" : "Not Ready");
    Serial.println();
}
