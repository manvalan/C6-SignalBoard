/*
 * SignalBoard v1.0 - REST API Endpoints
 * Stateless HTTP request handlers for web server
 * 
 * Design:
 * - All handlers are static (no state)
 * - JSON response formatting via ArduinoJson
 * - Stack-allocated JsonDocument (fixed size) to avoid fragmentation
 * - Input validation + error responses
 * 
 * Memory Strategy:
 * - Stack buffer: 512 bytes for JSON response (sufficient for all endpoints)
 * - No dynamic memory allocation beyond JsonDocument
 * - Reuse single buffer across requests (Web Server serializes)
 */

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "Config.h"
#include "Enums.h"
#include "Version.h"
#include "hardware/PCA9685_Driver.h"
#include "hardware/INA219_Driver.h"
#include "app/SignalManager.h"
#include "app/NVSConfig.h"
#include "app/MqttInterface.h"

class ApiEndpoints {
public:
    /**
     * GET /api/signals - List all loaded signals
     * Response: {"signals":[{"id":"sg1","type":0,"aspect":0},...]}
     */
    static String handleGetSignals(const SignalManager* mgr) {
        if (!mgr || !mgr->isReady()) {
            return "{\"error\":\"signals_not_ready\"}";
        }

        JsonDocument doc;
        JsonArray sig_array = doc["signals"].to<JsonArray>();

        uint8_t count = mgr->getSignalCount();
        for (uint8_t i = 0; i < count; i++) {
            const SignalDevice* device = mgr->getSignalByIndex(i);
            if (!device) {
                continue;
            }

            JsonObject sig = sig_array.add<JsonObject>();
            sig["index"] = i;
            sig["id"] = device->getId();
            sig["type"] = (int)device->getType();
            sig["aspect"] = (int)device->getCurrentAspect();
        }

        String result;
        serializeJson(doc, result);
        return result;
    }

    /**
     * GET /api/signals/{id} - Get single signal status
     * Response: {"id":"sg1","type":0,"aspect":0}
     */
    static String handleGetSignal(const SignalManager* mgr,
                                  const String& signal_id) {
        if (!mgr || !mgr->isReady()) {
            return "{\"error\":\"signals_not_ready\"}";
        }

        JsonDocument doc;

        if (signal_id.length() == 0) {
            doc["error"] = "missing_signal_id";
            String result;
            serializeJson(doc, result);
            return result;
        }

        const SignalDevice* device = mgr->findById(signal_id);
        if (!device) {
            doc["error"] = "signal_not_found";
            doc["id"] = signal_id;
            String result;
            serializeJson(doc, result);
            return result;
        }

        doc["id"] = device->getId();
        doc["type"] = (int)device->getType();
        doc["aspect"] = (int)device->getCurrentAspect();
        doc["status"] = "ok";

        String result;
        serializeJson(doc, result);
        return result;
    }

    /**
     * POST /api/signals/{id}/aspect - Set signal aspect
     * Request body: {"aspect":1}
     * Response: {"id":"sg1","aspect":1,"status":"ok"}
     */
    static String handleSetAspect(SignalManager* mgr, const String& signal_id,
                                  uint8_t aspect_value) {
        if (!mgr || !mgr->isReady()) {
            return "{\"error\":\"signals_not_ready\"}";
        }

        if (signal_id.length() == 0 || aspect_value > 5) {
            JsonDocument doc;
            doc["error"] = "invalid_parameters";
            String result;
            serializeJson(doc, result);
            return result;
        }

        SignalAspect aspect = (SignalAspect)aspect_value;
        bool success = mgr->setAspect(signal_id, aspect);

        JsonDocument doc;
        doc["id"] = signal_id;
        doc["aspect"] = (int)aspect;
        doc["status"] = success ? "ok" : "signal_not_found";

        String result;
        serializeJson(doc, result);
        return result;
    }

    /**
     * GET /api/system - System status + diagnostics
     * Response: {
     *   "firmware":"1.0.0",
     *   "uptime_ms":12345,
     *   "heap_free":45678,
     *   "wifi_status":"connected",
     *   "mqtt_status":"connected",
     *   "signals_loaded":5
     * }
     */
    static String handleSystemInfo(const SignalManager* mgr,
                                   const MqttInterface* mqtt,
                                   uint32_t uptime_ms,
                                   const INA219_Driver* power = nullptr) {
        JsonDocument doc;

        doc["firmware"] = FW_VERSION_STRING;
        doc["uptime_ms"] = uptime_ms;
        doc["heap_free"] = ESP.getFreeHeap();
        doc["heap_total"] = ESP.getHeapSize();
        doc["heap_largest_free"] = ESP.getMaxAllocHeap();

        // WiFi status
        int wifi_status = WiFi.status();
        if (wifi_status == WL_CONNECTED) {
            doc["wifi_status"] = "connected";
            doc["wifi_ip"] = WiFi.localIP().toString();
            doc["wifi_rssi"] = WiFi.RSSI();
        } else if (wifi_status == WL_NO_SSID_AVAIL) {
            doc["wifi_status"] = "no_ssid";
        } else if (wifi_status == WL_CONNECT_FAILED) {
            doc["wifi_status"] = "connect_failed";
        } else if (wifi_status == WL_IDLE_STATUS) {
            doc["wifi_status"] = "idle";
        } else {
            doc["wifi_status"] = "disconnected";
        }

        // MQTT status
        if (mqtt) {
            MqttState state = mqtt->getState();
            switch (state) {
                case MqttState::CONNECTED:
                    doc["mqtt_status"] = "connected";
                    break;
                case MqttState::CONNECTING:
                    doc["mqtt_status"] = "connecting";
                    break;
                case MqttState::DISCONNECTED:
                    doc["mqtt_status"] = "disconnected";
                    break;
                default:
                    doc["mqtt_status"] = "unknown";
            }
        } else {
            doc["mqtt_status"] = "not_initialized";
        }

        // Signal status
        if (mgr && mgr->isReady()) {
            doc["signals_loaded"] = mgr->getSignalCount();
            doc["signals_max"] = mgr->getMaxSignals();
        } else {
            doc["signals_loaded"] = 0;
            doc["signals_max"] = Config::MAX_SIGNALS;
        }

        // Power monitor (INA219)
        JsonObject pwr = doc["power"].to<JsonObject>();
        if (power && power->isInitialized()) {
            pwr["available"] = true;
            pwr["bus_voltage_v"] =
                roundf(power->getBusVoltage_V() * 100.0f) / 100.0f;
            pwr["current_ma"] = roundf(power->getCurrent_mA() * 10.0f) / 10.0f;
            pwr["power_mw"] = roundf(power->getPower_mW() * 10.0f) / 10.0f;
        } else {
            pwr["available"] = false;
        }

        doc["build_date"] = FW_BUILD_DATE;
        doc["build_time"] = FW_BUILD_TIME;
        doc["target"] = HW_TARGET;

        String result;
        serializeJson(doc, result);
        return result;
    }

    /**
     * POST /api/config/network - Update network configuration
     * Request body: {
     *   "hostname":"signal",
     *   "ssid":"MyWiFi",
     *   "password":"secret",
     *   "mqtt_host":"broker.local"
     * }
     * Response: {"status":"ok"}
     */
    static String handleConfigNetwork(NVSConfig* nvs,
                                      const String& json_request) {
        if (!nvs || json_request.length() == 0) {
            return "{\"error\":\"invalid_request\"}";
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, json_request);

        if (error) {
            return "{\"error\":\"json_parse_failed\"}";
        }

        // Update hostname if provided
        if (doc.containsKey("hostname") && doc["hostname"].is<const char*>()) {
            String hostname = doc["hostname"].as<String>();
            if (hostname.length() > 0 && hostname.length() <= 32) {
                nvs->writeHostname(hostname);
            }
        }

        // Update SSID if provided
        if (doc.containsKey("ssid") && doc["ssid"].is<const char*>()) {
            String ssid = doc["ssid"].as<String>();
            if (ssid.length() > 0) {
                nvs->writeSSID(ssid);
            }
        }

        // Update WiFi password if provided
        if (doc.containsKey("password") && doc["password"].is<const char*>()) {
            String password = doc["password"].as<String>();
            nvs->writeWiFiPassword(password);
        }

        // Update MQTT host if provided
        if (doc.containsKey("mqtt_host") && doc["mqtt_host"].is<const char*>()) {
            String broker = doc["mqtt_host"].as<String>();
            if (broker.length() > 0) {
                nvs->writeMqttHost(broker);
            }
        }

        JsonDocument response;
        response["status"] = "ok";
        response["message"] = "Configuration saved. Please restart.";

        String result;
        serializeJson(response, result);
        return result;
    }

    /**
     * GET /api/config - Read current network configuration
     * Response: {"hostname":"signal","ssid":"MyWiFi","mqtt_host":"broker.local"}
     */
    static String handleGetConfig(const NVSConfig* nvs) {
        if (!nvs) {
            return "{\"error\":\"config_not_available\"}";
        }

        JsonDocument doc;

        doc["hostname"] = nvs->readHostname(Config::DEFAULT_HOSTNAME);
        doc["ssid"] = nvs->readSSID("");
        doc["mqtt_host"] = nvs->readMqttHost(Config::DEFAULT_MQTT_HOST);
        // WiFi/web passwords are intentionally never exposed via the API
        doc["status"] = "ok";

        String result;
        serializeJson(doc, result);
        return result;
    }

    /**
     * GET /api/config/signals - Read all 5 signal slots from NVS
     * Response: {"slots":[
     *   {"index":0,"configured":true,"id":"sg1","type":0,
     *    "pins":[0,1,2],"brightness":[4095,4095,4095]},
     *   {"index":1,"configured":false}, ...]}
     */
    static String handleGetSignalSlots(const NVSConfig* nvs) {
        if (!nvs) {
            return "{\"error\":\"config_not_available\"}";
        }

        JsonDocument doc;
        JsonArray slots = doc["slots"].to<JsonArray>();

        for (uint8_t i = 0; i < Config::MAX_SIGNALS; i++) {
            JsonObject slot = slots.add<JsonObject>();
            slot["index"] = i;

            String id;
            SignalType type;
            uint8_t pins[3];
            uint16_t brightness[3];

            if (nvs->readSignal(i, id, type, pins, brightness)) {
                slot["configured"] = true;
                slot["id"] = id;
                slot["type"] = (int)type;

                JsonArray p = slot["pins"].to<JsonArray>();
                JsonArray b = slot["brightness"].to<JsonArray>();
                for (uint8_t c = 0; c < 3; c++) {
                    p.add(pins[c]);
                    b.add(brightness[c]);
                }
            } else {
                slot["configured"] = false;
            }
        }

        String result;
        serializeJson(doc, result);
        return result;
    }

    /**
     * POST /api/config/signals/{index} - Write a signal slot to NVS
     * Request body: {"id":"sg1","type":0,"pins":[0,1,2],
     *                "brightness":[4095,4095,4095]}
     * Brightness is optional (defaults to max).
     * Applies the change immediately by reloading the SignalManager.
     */
    static String handleSetSignalSlot(NVSConfig* nvs, SignalManager* mgr,
                                      uint8_t index, const String& body) {
        if (!nvs || index >= Config::MAX_SIGNALS) {
            return errorResponse("invalid_slot_index");
        }

        JsonDocument doc;
        if (deserializeJson(doc, body)) {
            return errorResponse("json_parse_failed");
        }

        String id = doc["id"] | "";
        int type = doc["type"] | 0;
        if (id.length() == 0 || id.length() > 32 || type < 0 || type > 1) {
            return errorResponse("invalid_parameters",
                                 "id (1-32 chars) and type (0|1) required");
        }

        uint8_t pins[3];
        uint16_t brightness[3];
        if (!extractChannelArrays(doc, pins, brightness)) {
            return errorResponse("invalid_parameters",
                                 "pins must be 3 distinct values 0-15");
        }

        nvs->writeSignal(index, id, (SignalType)type, pins, brightness);

        uint8_t loaded = mgr ? mgr->reload() : 0;

        JsonDocument resp;
        resp["status"] = "ok";
        resp["index"] = index;
        resp["id"] = id;
        resp["signals_loaded"] = loaded;

        String result;
        serializeJson(resp, result);
        return result;
    }

    /**
     * DELETE /api/config/signals/{index} - Clear a signal slot
     */
    static String handleDeleteSignalSlot(NVSConfig* nvs, SignalManager* mgr,
                                         uint8_t index) {
        if (!nvs || index >= Config::MAX_SIGNALS) {
            return errorResponse("invalid_slot_index");
        }

        nvs->clearSignal(index);
        uint8_t loaded = mgr ? mgr->reload() : 0;

        JsonDocument resp;
        resp["status"] = "ok";
        resp["index"] = index;
        resp["signals_loaded"] = loaded;

        String result;
        serializeJson(resp, result);
        return result;
    }

    /**
     * POST /api/system/restart - Reboot ESP32
     * Response: {"status":"restarting"}
     */
    static String handleRestart() {
        JsonDocument doc;
        doc["status"] = "restarting";
        doc["uptime_ms"] = millis();

        String result;
        serializeJson(doc, result);

        // Schedule restart after response is sent
        delay(100);
        ESP.restart();

        return result;
    }

    /**
     * Helper: Send JSON error response
     */
    static String errorResponse(const String& error_code,
                                const String& message = "") {
        JsonDocument doc;
        doc["error"] = error_code;
        if (message.length() > 0) {
            doc["message"] = message;
        }

        String result;
        serializeJson(doc, result);
        return result;
    }

    /**
     * Helper: Send JSON OK response
     */
    static String okResponse(const String& message = "") {
        JsonDocument doc;
        doc["status"] = "ok";
        if (message.length() > 0) {
            doc["message"] = message;
        }

        String result;
        serializeJson(doc, result);
        return result;
    }

private:
    /**
     * Validate and extract pins[3] + brightness[3] from a slot config request
     * Pins must be 3 distinct PCA9685 channels (0-15).
     * Brightness is optional, clamped to 0-4095, defaults to max.
     */
    static bool extractChannelArrays(const JsonDocument& doc, uint8_t pins[3],
                                     uint16_t brightness[3]) {
        JsonArrayConst pin_array = doc["pins"].as<JsonArrayConst>();
        if (pin_array.isNull() || pin_array.size() != 3) {
            return false;
        }

        for (uint8_t c = 0; c < 3; c++) {
            int pin = pin_array[c] | -1;
            if (pin < 0 || pin > 15) {
                return false;
            }
            pins[c] = (uint8_t)pin;
        }

        if (pins[0] == pins[1] || pins[0] == pins[2] || pins[1] == pins[2]) {
            return false;
        }

        JsonArrayConst br_array = doc["brightness"].as<JsonArrayConst>();
        for (uint8_t c = 0; c < 3; c++) {
            int value = Config::PWM_MAX_BRIGHTNESS;
            if (!br_array.isNull() && br_array.size() == 3) {
                value = br_array[c] | (int)Config::PWM_MAX_BRIGHTNESS;
            }
            if (value < 0) value = 0;
            if (value > Config::PWM_MAX_BRIGHTNESS) {
                value = Config::PWM_MAX_BRIGHTNESS;
            }
            brightness[c] = (uint16_t)value;
        }

        return true;
    }
};
