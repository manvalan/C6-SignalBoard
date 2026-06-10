/*
 * SignalBoard v1.0 - Global Configuration
 * Compile-time constants and build-time switches
 */

#pragma once

#include <cstdint>

namespace Config {

    // ===== Feature Flags =====
    constexpr bool ENABLE_MQTT = true;
    constexpr bool ENABLE_WEB_SERVER = true;
    constexpr bool ENABLE_OTA_UPDATES = true;
    constexpr bool ENABLE_MDNS = true;
    constexpr bool ENABLE_POWER_MONITOR = true;   // INA219 integration
    constexpr bool ENABLE_STATUS_LED = true;
    
    // ===== Hardware Configuration =====
    constexpr uint8_t MAX_SIGNALS = 5;            // Max 5 signal devices (15 PWM channels)
    constexpr uint8_t PWM_CHANNELS_PER_SIGNAL = 3; // Red, Yellow, Green
    constexpr uint8_t I2C_BUS_TIMEOUT_MS = 100;
    
    // ===== MQTT Configuration =====
    constexpr uint16_t MQTT_PORT = 1883;
    constexpr uint16_t MQTT_RECONNECT_INTERVAL_MS = 15000; // 15 seconds
    constexpr uint16_t MQTT_KEEP_ALIVE_SEC = 60;
    constexpr const char* DEFAULT_MQTT_HOST = "plastico.local";
    constexpr const char* MQTT_TOPIC_SUBSCRIBE = "rocrail/service/info/sg";
    constexpr const char* MQTT_TOPIC_PUBLISH = "rocrail/service/client";
    constexpr const char* MQTT_TOPIC_LWT = "railway/status/segnali";
    
    // ===== WiFi Configuration =====
    constexpr uint16_t WIFI_CONNECT_TIMEOUT_MS = 10000; // 10 seconds
    constexpr uint8_t WIFI_CONNECT_ATTEMPTS = 20;
    constexpr const char* DEFAULT_HOSTNAME = "signal";
    constexpr const char* DEFAULT_AP_SSID_PREFIX = "Setup-";
    
    // ===== Web Server Configuration =====
    constexpr uint16_t WEB_SERVER_PORT = 80;
    constexpr const char* DEFAULT_WEB_USERNAME = "admin";
    constexpr const char* DEFAULT_WEB_PASSWORD = "signal";
    constexpr uint16_t WEB_INACTIVITY_TIMEOUT_MS = 30000; // 30 seconds
    
    // ===== PWM Configuration =====
    constexpr uint16_t PWM_FREQUENCY_HZ = 1000; // 1 kHz for LED brightness
    constexpr uint16_t PWM_MAX_BRIGHTNESS = 4095; // 12-bit resolution
    constexpr uint16_t PWM_DEFAULT_BRIGHTNESS = 4095; // Max by default
    
    // ===== Timing & Watchdog =====
    constexpr uint32_t LOOP_HEARTBEAT_INTERVAL_MS = 5000; // 5 seconds
    constexpr uint16_t WATCHDOG_TIMEOUT_SEC = 10;
    
    // ===== Debugging =====
    constexpr bool ENABLE_DEBUG_SERIAL = true;
    constexpr bool ENABLE_DEBUG_MQTT = true;
    constexpr bool ENABLE_DEBUG_WEB = false; // Can be verbose
    
    // ===== NVS Keys (for compatibility with legacy code) =====
    constexpr const char* NVS_KEY_HOSTNAME = "hostname";
    constexpr const char* NVS_KEY_WEB_PASSWORD = "web_pass";
    constexpr const char* NVS_KEY_SSID = "ssid";
    constexpr const char* NVS_KEY_WIFI_PASSWORD = "password";
    constexpr const char* NVS_KEY_MQTT_HOST = "mqtt_host";
    
    // Signal configuration keys
    constexpr const char* NVS_KEY_SIGNAL_ID_FMT = "id_%d";       // e.g., "id_1"
    constexpr const char* NVS_KEY_SIGNAL_TYPE_FMT = "tipo_%d";   // e.g., "tipo_1"
    constexpr const char* NVS_KEY_SIGNAL_PIN_R_FMT = "pinR_%d";  // Red pin
    constexpr const char* NVS_KEY_SIGNAL_PIN_G_FMT = "pinG_%d";  // Yellow pin
    constexpr const char* NVS_KEY_SIGNAL_PIN_V_FMT = "pinV_%d";  // Green pin
    constexpr const char* NVS_KEY_SIGNAL_BR_R_FMT = "brR_%d";    // Red brightness
    constexpr const char* NVS_KEY_SIGNAL_BR_G_FMT = "brG_%d";    // Yellow brightness
    constexpr const char* NVS_KEY_SIGNAL_BR_V_FMT = "brV_%d";    // Green brightness

} // namespace Config
