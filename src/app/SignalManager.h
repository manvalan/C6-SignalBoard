/*
 * SignalBoard v1.0 - Signal Manager
 * Central hub for managing up to 5 signal devices
 * 
 * Responsibilities:
 * - Load/save signals from NVS
 * - Provide ID-based signal lookup
 * - Dispatch aspect changes to hardware
 * - Generate JSON status for API
 */

#pragma once

#include <Arduino.h>
#include <vector>
#include "../include/Enums.h"
#include "../hardware/SignalDevice.h"
#include "../hardware/PCA9685_Driver.h"
#include "NVSConfig.h"

class SignalManager {
private:
    std::vector<SignalDevice*> signals;
    PCA9685_Driver* pca_driver;
    NVSConfig* nvsConfig;
    uint8_t max_signals;

public:
    explicit SignalManager(PCA9685_Driver* pca, NVSConfig* nvs,
                          uint8_t max = 5)
        : pca_driver(pca), nvsConfig(nvs), max_signals(max) {}

    ~SignalManager() {
        for (auto sig : signals) {
            delete sig;
        }
        signals.clear();
    }

    /**
     * Load all signals from NVS
     * Creates SignalDevice objects for each configured signal
     * @return number of signals loaded
     */
    uint8_t loadFromNVS() {
        uint8_t loaded = 0;

        if (!pca_driver || !pca_driver->isInitialized()) {
            Serial.println("[SignalManager] ERROR: PCA9685 not initialized");
            return 0;
        }

        for (uint8_t i = 0; i < max_signals; i++) {
            String id;
            SignalType type;
            uint8_t pins[3];
            uint16_t brightness[3];

            if (nvsConfig->readSignal(i, id, type, pins, brightness)) {
                auto device = new SignalDevice(id, type, pins[0],
                                               pins[1], pins[2],
                                               pca_driver);

                device->setBrightness(0, brightness[0]);
                device->setBrightness(1, brightness[1]);
                device->setBrightness(2, brightness[2]);

                signals.push_back(device);
                loaded++;

                Serial.printf("[SignalManager] Loaded signal %d: id=%s, "
                              "type=%d\n",
                              i, id.c_str(), (int)type);
            }
        }

        return loaded;
    }

    /**
     * Find signal by Rocrail ID
     * @param id Signal ID (e.g., "sg1")
     * @return Pointer to SignalDevice, or nullptr if not found
     */
    SignalDevice* findById(const String& id) const {
        for (auto sig : signals) {
            if (sig->getId() == id) {
                return sig;
            }
        }
        return nullptr;
    }

    /**
     * Set aspect for signal by ID
     * @param id Signal ID
     * @param aspect Target aspect
     * @return true if signal found and aspect changed
     */
    bool setAspect(const String& id, SignalAspect aspect) {
        SignalDevice* sig = findById(id);
        if (!sig) {
            Serial.printf("[SignalManager] Signal '%s' not found\n",
                         id.c_str());
            return false;
        }

        return sig->setAspect(aspect);
    }

    /**
     * Generate JSON status for all signals
     * Format: {"signals":[{"id":"sg1","type":0,"aspect":0},...]}
     */
    String getStatusJSON() const {
        String json = "{\"signals\":[";

        for (size_t i = 0; i < signals.size(); i++) {
            const auto sig = signals[i];
            json += "{\"id\":\"" + sig->getId() + "\",";
            json += "\"type\":" + String((int)sig->getType()) + ",";
            json += "\"aspect\":" + String((int)sig->getCurrentAspect()) +
                    "}";

            if (i < signals.size() - 1) {
                json += ",";
            }
        }

        json += "]}";
        return json;
    }

    /**
     * Reload all signals from NVS (after a config change)
     * Turns every PWM channel off, drops current devices and re-reads NVS
     * @return number of signals loaded
     */
    uint8_t reload() {
        if (pca_driver && pca_driver->isInitialized()) {
            pca_driver->allOff();
        }

        for (auto sig : signals) {
            delete sig;
        }
        signals.clear();

        return loadFromNVS();
    }

    /**
     * Get signal by index (0-based)
     * @return Pointer to SignalDevice, or nullptr if out of range
     */
    SignalDevice* getSignalByIndex(uint8_t index) const {
        if (index >= signals.size()) {
            return nullptr;
        }
        return signals[index];
    }

    /**
     * Get count of loaded signals
     */
    uint8_t getSignalCount() const { return signals.size(); }

    /**
     * Get maximum allowed signals
     */
    uint8_t getMaxSignals() const { return max_signals; }

    /**
     * Check if manager is ready
     */
    bool isReady() const {
        return pca_driver && pca_driver->isInitialized() &&
               signals.size() > 0;
    }
};
