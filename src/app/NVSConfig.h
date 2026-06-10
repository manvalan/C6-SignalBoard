/*
 * SignalBoard v1.0 - NVS Configuration Manager
 * Preserve legacy NVS namespace approach while adding signal management
 * 
 * Namespaces:
 * - "network": hostname, SSID, password, MQTT host
 * - "railway": 5 signal configurations (id, type, pins, brightness)
 */

#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "../include/Enums.h"
#include <cstdint>

class NVSConfig {
private:
    String ns_network;
    String ns_railway;

public:
    NVSConfig() : ns_network("network"), ns_railway("railway") {}

    // ===== NETWORK SETTINGS =====
    
    String readHostname(const String& default_val = "signal") const {
        Preferences prefs;
        prefs.begin(ns_network.c_str(), true);
        String val = prefs.getString("hostname", default_val);
        prefs.end();
        return val;
    }

    void writeHostname(const String& hostname) {
        Preferences prefs;
        prefs.begin(ns_network.c_str(), false);
        prefs.putString("hostname", hostname);
        prefs.end();
    }

    String readSSID(const String& default_val = "") const {
        Preferences prefs;
        prefs.begin(ns_network.c_str(), true);
        String val = prefs.getString("ssid", default_val);
        prefs.end();
        return val;
    }

    void writeSSID(const String& ssid) {
        Preferences prefs;
        prefs.begin(ns_network.c_str(), false);
        prefs.putString("ssid", ssid);
        prefs.end();
    }

    String readWiFiPassword(const String& default_val = "") const {
        Preferences prefs;
        prefs.begin(ns_network.c_str(), true);
        String val = prefs.getString("password", default_val);
        prefs.end();
        return val;
    }

    void writeWiFiPassword(const String& password) {
        Preferences prefs;
        prefs.begin(ns_network.c_str(), false);
        prefs.putString("password", password);
        prefs.end();
    }

    String readMqttHost(const String& default_val = "plastico.local") const {
        Preferences prefs;
        prefs.begin(ns_network.c_str(), true);
        String val = prefs.getString("mqtt_host", default_val);
        prefs.end();
        return val;
    }

    void writeMqttHost(const String& mqtt_host) {
        Preferences prefs;
        prefs.begin(ns_network.c_str(), false);
        prefs.putString("mqtt_host", mqtt_host);
        prefs.end();
    }

    String readWebPassword(const String& default_val = "signal") const {
        Preferences prefs;
        prefs.begin(ns_network.c_str(), true);
        String val = prefs.getString("web_pass", default_val);
        prefs.end();
        return val;
    }

    void writeWebPassword(const String& password) {
        Preferences prefs;
        prefs.begin(ns_network.c_str(), false);
        prefs.putString("web_pass", password);
        prefs.end();
    }

    // ===== RAILWAY SIGNAL CONFIG =====
    
    /**
     * Read signal configuration from NVS
     * @param index Signal index (0-4)
     * @param out_id Reference to String for signal ID
     * @param out_type Reference to SignalType
     * @param out_pins Array of 3 uint8_t for [Red, Yellow, Green] pins
     * @param out_brightness Array of 3 uint16_t for brightness values
     * @return true if configuration found, false if empty slot
     */
    bool readSignal(uint8_t index, String& out_id, SignalType& out_type,
                    uint8_t out_pins[3], uint16_t out_brightness[3]) const {
        if (index >= 5) return false;

        Preferences prefs;
        prefs.begin(ns_railway.c_str(), true);

        String id_key = "id_" + String(index + 1);
        out_id = prefs.getString(id_key.c_str(), "");

        if (out_id == "") {
            prefs.end();
            return false;  // Slot is empty
        }

        String type_key = "tipo_" + String(index + 1);
        out_type = (SignalType)prefs.getInt(type_key.c_str(), 0);

        String pin_r = "pinR_" + String(index + 1);
        String pin_g = "pinG_" + String(index + 1);
        String pin_v = "pinV_" + String(index + 1);
        out_pins[0] = (uint8_t)prefs.getInt(pin_r.c_str(), 0xFF);
        out_pins[1] = (uint8_t)prefs.getInt(pin_g.c_str(), 0xFF);
        out_pins[2] = (uint8_t)prefs.getInt(pin_v.c_str(), 0xFF);

        String br_r = "brR_" + String(index + 1);
        String br_g = "brG_" + String(index + 1);
        String br_v = "brV_" + String(index + 1);
        out_brightness[0] = prefs.getInt(br_r.c_str(), 4095);
        out_brightness[1] = prefs.getInt(br_g.c_str(), 4095);
        out_brightness[2] = prefs.getInt(br_v.c_str(), 4095);

        prefs.end();
        return true;
    }

    /**
     * Write signal configuration to NVS
     */
    void writeSignal(uint8_t index, const String& id, SignalType type,
                     const uint8_t pins[3], const uint16_t brightness[3]) {
        if (index >= 5) return;

        Preferences prefs;
        prefs.begin(ns_railway.c_str(), false);

        prefs.putString(("id_" + String(index + 1)).c_str(), id);
        prefs.putInt(("tipo_" + String(index + 1)).c_str(), (int)type);
        prefs.putInt(("pinR_" + String(index + 1)).c_str(), pins[0]);
        prefs.putInt(("pinG_" + String(index + 1)).c_str(), pins[1]);
        prefs.putInt(("pinV_" + String(index + 1)).c_str(), pins[2]);
        prefs.putInt(("brR_" + String(index + 1)).c_str(), brightness[0]);
        prefs.putInt(("brG_" + String(index + 1)).c_str(), brightness[1]);
        prefs.putInt(("brV_" + String(index + 1)).c_str(), brightness[2]);

        prefs.end();
    }

    /**
     * Clear a signal slot (factory reset for one slot)
     */
    void clearSignal(uint8_t index) {
        if (index >= 5) return;

        Preferences prefs;
        prefs.begin(ns_railway.c_str(), false);

        prefs.remove(("id_" + String(index + 1)).c_str());
        prefs.remove(("tipo_" + String(index + 1)).c_str());
        prefs.remove(("pinR_" + String(index + 1)).c_str());
        prefs.remove(("pinG_" + String(index + 1)).c_str());
        prefs.remove(("pinV_" + String(index + 1)).c_str());
        prefs.remove(("brR_" + String(index + 1)).c_str());
        prefs.remove(("brG_" + String(index + 1)).c_str());
        prefs.remove(("brV_" + String(index + 1)).c_str());

        prefs.end();
    }

    /**
     * Factory reset: clear all data
     */
    void factoryReset() {
        Preferences prefs;

        prefs.begin(ns_network.c_str(), false);
        prefs.clear();
        prefs.end();

        prefs.begin(ns_railway.c_str(), false);
        prefs.clear();
        prefs.end();

        Serial.println("[NVS] Factory reset complete");
    }
};
