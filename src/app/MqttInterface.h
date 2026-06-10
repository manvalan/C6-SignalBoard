/*
 * SignalBoard v1.0 - MQTT Interface
 * Rocrail protocol integration via PubSubClient
 * 
 * Topics:
 * - Subscribe: rocrail/service/info/sg
 * - Publish: rocrail/service/client
 * - LWT: railway/status/segnali
 */

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESPmDNS.h>
#include "../include/Config.h"
#include "../include/Enums.h"
#include "SignalManager.h"
#include "RocRailParser.h"
#include <cstring>

class MqttInterface {
private:
    WiFiClient espClient;
    PubSubClient mqttClient;
    SignalManager* signalMgr;
    String broker_host;
    IPAddress broker_ip;
    MqttState state;
    unsigned long lastReconnectAttempt;
    String clientId;

public:
    explicit MqttInterface(SignalManager* sig_mgr)
        : mqttClient(espClient), signalMgr(sig_mgr),
          state(MqttState::DISCONNECTED), lastReconnectAttempt(0) {
        // Generate unique client ID from MAC address
        uint64_t mac = ESP.getEfuseMac();
        clientId = "signal_" + String((uint32_t)(mac >> 32), HEX) + 
                   String((uint32_t)mac, HEX);
        mqttClient.setCallback([this](char* topic, byte* payload,
                                      unsigned int length) {
            this->onMessageReceived(topic, payload, length);
        });
    }

    /**
     * Initialize MQTT interface with broker configuration
     * @param broker Broker hostname (e.g., "plastico.local")
     * @return true if broker resolved successfully
     */
    bool init(const String& broker) {
        broker_host = broker;

        if (!resolveBrokerIP()) {
            Serial.println("[MQTT] Failed to resolve broker IP");
            return false;
        }

        Serial.printf("[MQTT] Broker IP: %s\n", broker_ip.toString().c_str());
        mqttClient.setServer(broker_ip, Config::MQTT_PORT);
        return true;
    }

    /**
     * Main MQTT loop - handle connection/disconnection/messages
     * Call this from main loop()
     */
    void loop() {
        if (state == MqttState::CONNECTED) {
            if (!mqttClient.connected()) {
                state = MqttState::DISCONNECTED;
                Serial.println("[MQTT] Disconnected");
            } else {
                mqttClient.loop();
            }
        } else if (state == MqttState::DISCONNECTED) {
            unsigned long now = millis();
            if (now - lastReconnectAttempt >= Config::MQTT_RECONNECT_INTERVAL_MS) {
                lastReconnectAttempt = now;
                attemptConnect();
            }
        }
    }

    /**
     * Send signal feedback to Rocrail
     * Publishes: <sg id="sg1" cmd="green"/>
     */
    void sendFeedback(const String& signal_id, SignalAspect aspect) {
        if (state != MqttState::CONNECTED) {
            return;
        }

        String cmd_str = aspectToCommandString(aspect);
        String xml = "<sg id=\"" + signal_id + "\" cmd=\"" + cmd_str + "\"/>";

        mqttClient.publish(Config::MQTT_TOPIC_PUBLISH, xml.c_str());

        Serial.printf("[MQTT] Feedback: %s\n", xml.c_str());
    }

    /**
     * Check if MQTT is connected
     */
    bool isConnected() const { return state == MqttState::CONNECTED; }

    MqttState getState() const { return state; }

private:
    /**
     * Resolve broker hostname to IP address
     */
    bool resolveBrokerIP() {
        if (broker_ip.fromString(broker_host)) {
            return true;  // Already an IP address
        }

        String hostname = broker_host;
        if (hostname.endsWith(".local")) {
            hostname = hostname.substring(0, hostname.length() - 6);
        }

        broker_ip = MDNS.queryHost(hostname);
        return broker_ip.toString() != "0.0.0.0";
    }

    /**
     * Attempt MQTT connection with LWT
     */
    void attemptConnect() {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[MQTT] WiFi not connected yet");
            return;
        }

        state = MqttState::CONNECTING;
        Serial.printf("[MQTT] Attempting connection as %s...\n",
                      clientId.c_str());

        String lwt_msg = "{\"module\":\"" + clientId +
                         "\",\"status\":\"offline\"}";
        String online_msg = "{\"module\":\"" + clientId +
                            "\",\"status\":\"online\"}";

        if (mqttClient.connect(clientId.c_str(), NULL, NULL,
                               Config::MQTT_TOPIC_LWT, 0, true,
                               lwt_msg.c_str())) {
            state = MqttState::CONNECTED;
            Serial.println("[MQTT] Connected with LWT");

            mqttClient.subscribe(Config::MQTT_TOPIC_SUBSCRIBE);
            mqttClient.publish(Config::MQTT_TOPIC_LWT, online_msg.c_str(),
                              true);

            if (signalMgr && signalMgr->isReady()) {
                mqttClient.publish(Config::MQTT_TOPIC_PUBLISH,
                                  signalMgr->getStatusJSON().c_str());
            }
        } else {
            state = MqttState::DISCONNECTED;
            Serial.printf("[MQTT] Connection failed, rc=%d\n",
                         mqttClient.state());
        }
    }

    /**
     * Handle incoming MQTT message
     */
    void onMessageReceived(char* topic, byte* payload, unsigned int length) {
        char msgBuffer[length + 1];
        memcpy(msgBuffer, payload, length);
        msgBuffer[length] = '\0';
        String msg = String(msgBuffer);

        Serial.printf("[MQTT] Received on %s: %s\n", topic, msg.c_str());

        if (msg.indexOf("<sg ") != -1 && signalMgr) {
            String signal_id;
            SignalAspect aspect;

            // Determine signal type (simplified - default to MAIN)
            SignalDevice* sig = signalMgr->findById(signal_id);
            SignalType type = sig ? sig->getType() : SignalType::MAIN;

            if (RocRailParser::parseSignalCommand(msg, signal_id, aspect,
                                                  type)) {
                if (signalMgr->setAspect(signal_id, aspect)) {
                    sendFeedback(signal_id, aspect);
                }
            }
        }
    }

    /**
     * Map aspect to XML command string
     */
    String aspectToCommandString(SignalAspect aspect) const {
        switch (aspect) {
            case SignalAspect::ASPECT_RED:
            case SignalAspect::ASPECT_STOP:
                return "red";
            case SignalAspect::ASPECT_GREEN:
            case SignalAspect::ASPECT_GO:
                return "green";
            case SignalAspect::ASPECT_YELLOW:
            case SignalAspect::ASPECT_OBLIQUE:
                return "yellow";
            default:
                return "red";
        }
    }
};
