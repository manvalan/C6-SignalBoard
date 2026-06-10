/*
 * SignalBoard v1.0 - ESP32-C6 Firmware
 * Main entry point: setup() and loop()
 * 
 * Phase 1: Hardware initialization ✓
 * Phase 2: NVS config, signal management, MQTT integration (current)
 * Phase 3: Web UI + LittleFS
 * Phase 4: ESP32-C6 native + OTA
 */

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "../include/Pins.h"
#include "../include/Config.h"
#include "../include/Version.h"
#include "hardware/I2C_HAL.h"
#include "hardware/PCA9685_Driver.h"
#include "hardware/SignalDevice.h"
#include "app/NVSConfig.h"
#include "app/SignalManager.h"
#include "app/MqttInterface.h"
#include "app/WebServer.h"

// ============================================================================
// Hardware instances
// ============================================================================
I2C_HAL i2c_bus(BoardPins::I2C_SDA, BoardPins::I2C_SCL);
PCA9685_Driver pca9685(&i2c_bus, BoardPins::PCA_ADDR_PRIMARY);

// Application layer
NVSConfig nvsConfig;
SignalManager* signalManager = nullptr;
MqttInterface* mqttInterface = nullptr;
WebServerManager* webServer = nullptr;

// State tracking
uint32_t last_status_print = 0;
uint32_t last_wifi_check = 0;

// ============================================================================
// Forward declarations
// ============================================================================
void initializeHardware();
void initializeApplication();
void updateStatusLED();
void handleWiFiConnection();
void printSystemInfo();

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

    // Phase 1: Initialize hardware
    initializeHardware();
    
    // Phase 2: Initialize application layer
    initializeApplication();
    
    // Phase 3: Initialize web server (after hardware/application ready)
    Serial.println("[INIT] Starting web server on port 80...");
    webServer = new WebServerManager(signalManager, &nvsConfig, mqttInterface);
    if (webServer) {
        if (webServer->init()) {
            Serial.println("[OK] Web server initialized");
        } else {
            Serial.println("[ERROR] Web server initialization failed");
        }
    }
    
    Serial.println();
    Serial.println("[READY] SignalBoard ready for operation");
    Serial.println("Waiting for WiFi connection...");
    Serial.println();
}

// ============================================================================
// LOOP: Main event loop (called repeatedly)
// ============================================================================
void loop() {
    // WiFi management
    handleWiFiConnection();
    
    // MQTT loop (only if WiFi connected)
    if (WiFi.status() == WL_CONNECTED && mqttInterface) {
        mqttInterface->loop();
    }
    
    // Web server loop (handles HTTP requests)
    if (webServer) {
        webServer->loop();
    }
    
    // Status LED heartbeat
    updateStatusLED();
    
    // Periodic debug output
    if (Config::ENABLE_DEBUG_SERIAL) {
        uint32_t now = millis();
        if (now - last_status_print > 30000) {
            last_status_print = now;
            
            Serial.println();
            Serial.printf("[STATS] Uptime: %lu s\n",
                          (unsigned long)(now / 1000));
            Serial.printf("[STATS] Free heap: %u bytes\n", ESP.getFreeHeap());
            Serial.printf("[STATS] WiFi: %s\n", 
                         WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
            Serial.printf("[STATS] MQTT: %s\n", 
                         mqttInterface && mqttInterface->isConnected() ? "Connected" : "Disconnected");
            if (signalManager) {
                Serial.printf("[STATS] Signals loaded: %d/%d\n", 
                             signalManager->getSignalCount(),
                             signalManager->getMaxSignals());
            }
        }
    }
}

// ============================================================================
// Hardware initialization
// ============================================================================
void initializeHardware() {
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
}

// ============================================================================
// Application initialization
// ============================================================================
void initializeApplication() {
    Serial.println("[INIT] Loading NVS configuration...");
    
    // Create SignalManager
    signalManager = new SignalManager(&pca9685, &nvsConfig, Config::MAX_SIGNALS);
    
    if (!signalManager) {
        Serial.println("[ERROR] Failed to create SignalManager");
        return;
    }
    
    // Load signals from NVS
    uint8_t loaded = signalManager->loadFromNVS();
    Serial.printf("[OK] Loaded %d signal(s) from NVS\n", loaded);
    
    // Create MQTT interface
    mqttInterface = new MqttInterface(signalManager);
    if (!mqttInterface) {
        Serial.println("[ERROR] Failed to create MqttInterface");
        return;
    }
    
    Serial.println("[OK] Application layer initialized");
}

// ============================================================================
// Handle WiFi connection
// ============================================================================
void handleWiFiConnection() {
    uint32_t now = millis();
    
    if (now - last_wifi_check < 5000) {
        return;  // Check every 5 seconds
    }
    last_wifi_check = now;
    
    if (WiFi.status() == WL_CONNECTED) {
        return;  // Already connected
    }
    
    // Try to connect
    if (WiFi.status() == WL_IDLE_STATUS) {
        Serial.println("[WiFi] Attempting connection...");
        
        String ssid = nvsConfig.readSSID("");
        String password = nvsConfig.readWiFiPassword("");
        
        if (ssid.length() > 0) {
            WiFi.mode(WIFI_STA);
            WiFi.begin(ssid.c_str(), password.c_str());
        } else {
            Serial.println("[WiFi] No SSID configured, using AP mode");
            WiFi.mode(WIFI_AP);
            String hostname = nvsConfig.readHostname("signal");
            WiFi.softAP(hostname.c_str());
        }
        return;
    }
    
    // Connected: initialize MDNS and MQTT
    if (WiFi.status() == WL_CONNECTED) {
        static bool mqtt_initialized = false;
        
        if (!mqtt_initialized && mqttInterface) {
            String hostname = nvsConfig.readHostname("signal");
            String broker = nvsConfig.readMqttHost(Config::DEFAULT_MQTT_HOST);
            
            Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("[mDNS] Registering hostname: %s.local\n", hostname.c_str());
            
            if (!MDNS.begin(hostname.c_str())) {
                Serial.println("[mDNS] Failed to start mDNS responder");
            }
            
            if (mqttInterface->init(broker)) {
                Serial.printf("[MQTT] Initialized with broker: %s\n", broker.c_str());
            } else {
                Serial.println("[MQTT] Failed to initialize");
            }
            
            mqtt_initialized = true;
        }
    }
}

// ============================================================================
// Update status LED
// ============================================================================
void updateStatusLED() {
    if (!Config::ENABLE_STATUS_LED) {
        return;
    }
    
    static uint32_t last_blink = 0;
    static uint8_t state = 0;
    uint32_t now = millis();
    
    // Blink pattern: Fast when disconnected, slow when connected
    uint32_t interval = 1000;  // Default: 1 second period
    
    if (WiFi.status() == WL_CONNECTED) {
        interval = 200;  // Fast blink when connected
    }
    
    if (now - last_blink >= interval / 2) {
        last_blink = now;
        state = !state;
        digitalWrite(BoardPins::LED_STATUS, state);
    }
}

// ============================================================================
// Utility: Print system information
// ============================================================================
void printSystemInfo() {
    Serial.println("\n=== System Information ===");
    Serial.printf("Firmware: %s\n", FW_VERSION_STRING);
    Serial.printf("Hardware: %s (%s)\n", HW_TARGET, HW_VARIANT);
    Serial.printf("Build: %s %s\n", FW_BUILD_DATE, FW_BUILD_TIME);
    Serial.printf("Free Heap: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Uptime: %lu ms\n", millis());
    Serial.printf("PCA9685: %s\n", pca9685.isInitialized() ? "Ready" : "Not Ready");
    Serial.printf("I2C Bus: %s\n", i2c_bus.isInitialized() ? "Ready" : "Not Ready");
    
    if (signalManager) {
        Serial.printf("Signals: %d/%d loaded\n", 
                     signalManager->getSignalCount(),
                     signalManager->getMaxSignals());
    }
    
    Serial.printf("WiFi: %s\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    }
    
    if (mqttInterface) {
        Serial.printf("MQTT: %s\n", mqttInterface->isConnected() ? "Connected" : "Disconnected");
    }
    Serial.println();
}
