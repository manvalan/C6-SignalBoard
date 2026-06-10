/*
 * SignalBoard v1.0 - Web Server Manager
 * WebServer wrapper with REST API routing + static file serving
 * 
 * Responsibilities:
 * - Initialize HTTP server on port 80
 * - Register REST API routes
 * - Serve static files from LittleFS (index.html, etc)
 * - Add CORS headers to all responses
 * - Handle LittleFS initialization
 * 
 * Design:
 * - Single-threaded (no async requests)
 * - Non-blocking loop() for main event integration
 * - Stack-safe: minimal allocations in handlers
 * 
 * Cyclomatic Complexity: ≤5
 * Max Method Length: ≤30 lines
 */

#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <uri/UriBraces.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "../include/Config.h"
#include "../include/ApiEndpoints.h"
#include "SignalManager.h"
#include "NVSConfig.h"
#include "MqttInterface.h"

class WebServerManager {
private:
    WebServer server;
    SignalManager* signalMgr;
    NVSConfig* nvsConfig;
    MqttInterface* mqttInterface;
    bool isRunning;
    uint32_t startup_time;

    // CORS helper
    void setCorsHeaders() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Allow-Methods",
                         "GET, POST, PUT, DELETE, OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers",
                         "Content-Type, Authorization");
    }

    // OPTIONS request handler (CORS preflight)
    void handleOptions() {
        setCorsHeaders();
        server.send(200);
    }

    // Parse and validate a numeric slot index (0-4) from a path segment
    bool parseSlotIndex(const String& segment, uint8_t& out_index) const {
        if (segment.length() == 0 || segment.length() > 2) {
            return false;
        }
        for (size_t i = 0; i < segment.length(); i++) {
            if (!isDigit(segment[i])) {
                return false;
            }
        }
        long value = segment.toInt();
        if (value < 0 || value >= Config::MAX_SIGNALS) {
            return false;
        }
        out_index = (uint8_t)value;
        return true;
    }

public:
    explicit WebServerManager(SignalManager* sig_mgr, NVSConfig* nvs,
                              MqttInterface* mqtt)
        : server(Config::WEB_SERVER_PORT), signalMgr(sig_mgr),
          nvsConfig(nvs), mqttInterface(mqtt), isRunning(false),
          startup_time(0) {}

    /**
     * Initialize web server and mount LittleFS
     * @return true if successful
     */
    bool init() {
        // Initialize LittleFS
        Serial.println("[WebServer] Initializing LittleFS...");
        if (!LittleFS.begin()) {
            Serial.println("[WebServer] ERROR: LittleFS mount failed!");
            return false;
        }
        Serial.println("[WebServer] ✓ LittleFS mounted (1 MB)");

        // List files in LittleFS (debug)
        if (Config::ENABLE_DEBUG_SERIAL) {
            listLittleFSFiles();
        }

        // Register API routes
        registerApiRoutes();

        // Register static file serving
        registerStaticRoutes();

        // Register OPTIONS handler for CORS preflight
        server.onNotFound([this]() { handleNotFound(); });

        startup_time = millis();
        isRunning = true;

        Serial.printf("[WebServer] Ready on http://signal.local/\n");
        return true;
    }

    /**
     * Main server loop - call from loop()
     */
    void loop() {
        if (isRunning) {
            server.handleClient();
        }
    }

    /**
     * Check if server is running
     */
    bool running() const { return isRunning; }

    /**
     * Get uptime in milliseconds
     */
    uint32_t getUptimeMs() const {
        return millis() - startup_time;
    }

    /**
     * Stop the server
     */
    void stop() {
        if (isRunning) {
            server.stop();
            LittleFS.end();
            isRunning = false;
            Serial.println("[WebServer] Stopped");
        }
    }

private:
    /**
     * Register all REST API routes
     */
    void registerApiRoutes() {
        Serial.println("[WebServer] Registering API routes...");

        // GET /api/signals
        server.on("/api/signals", HTTP_GET, [this]() {
            setCorsHeaders();
            String response = ApiEndpoints::handleGetSignals(signalMgr);
            server.send(200, "application/json", response);
        });

        // GET /api/signals/status (alternative to /api/signals)
        server.on("/api/signals/status", HTTP_GET, [this]() {
            setCorsHeaders();
            if (signalMgr && signalMgr->isReady()) {
                server.send(200, "application/json",
                           signalMgr->getStatusJSON());
            } else {
                server.send(503, "application/json",
                           "{\"error\":\"signals_not_ready\"}");
            }
        });

        // GET /api/signals/[signal_id] - single signal status
        server.on(UriBraces("/api/signals/{}"), HTTP_GET, [this]() {
            setCorsHeaders();
            String response = ApiEndpoints::handleGetSignal(
                signalMgr, server.pathArg(0));
            server.send(200, "application/json", response);
        });

        // POST /api/signals/[signal_id]/aspect - set signal aspect
        server.on(UriBraces("/api/signals/{}/aspect"), HTTP_POST, [this]() {
            handleSignalAspectPost(server.pathArg(0));
        });

        // POST /api/signals/[signal_id] - alternative form
        server.on(UriBraces("/api/signals/{}"), HTTP_POST, [this]() {
            handleSignalAspectPost(server.pathArg(0));
        });

        // GET /api/system
        server.on("/api/system", HTTP_GET, [this]() {
            setCorsHeaders();
            String response = ApiEndpoints::handleSystemInfo(
                signalMgr, mqttInterface, getUptimeMs());
            server.send(200, "application/json", response);
        });

        // GET /api/config
        server.on("/api/config", HTTP_GET, [this]() {
            setCorsHeaders();
            String response = ApiEndpoints::handleGetConfig(nvsConfig);
            server.send(200, "application/json", response);
        });

        // POST /api/config/network
        server.on("/api/config/network", HTTP_POST, [this]() {
            setCorsHeaders();
            if (server.hasArg("plain")) {
                String body = server.arg("plain");
                String response =
                    ApiEndpoints::handleConfigNetwork(nvsConfig, body);
                server.send(200, "application/json", response);
            } else {
                server.send(400, "application/json",
                           "{\"error\":\"missing_body\"}");
            }
        });

        // GET /api/config/signals - all 5 slot configurations
        server.on("/api/config/signals", HTTP_GET, [this]() {
            setCorsHeaders();
            String response = ApiEndpoints::handleGetSignalSlots(nvsConfig);
            server.send(200, "application/json", response);
        });

        // POST /api/config/signals/[index] - write slot config
        server.on(UriBraces("/api/config/signals/{}"), HTTP_POST, [this]() {
            setCorsHeaders();
            uint8_t index;
            if (!parseSlotIndex(server.pathArg(0), index)) {
                server.send(400, "application/json",
                           "{\"error\":\"invalid_slot_index\"}");
                return;
            }
            if (!server.hasArg("plain")) {
                server.send(400, "application/json",
                           "{\"error\":\"missing_body\"}");
                return;
            }
            String response = ApiEndpoints::handleSetSignalSlot(
                nvsConfig, signalMgr, index, server.arg("plain"));
            server.send(200, "application/json", response);
        });

        // DELETE /api/config/signals/[index] - clear slot
        server.on(UriBraces("/api/config/signals/{}"), HTTP_DELETE, [this]() {
            setCorsHeaders();
            uint8_t index;
            if (!parseSlotIndex(server.pathArg(0), index)) {
                server.send(400, "application/json",
                           "{\"error\":\"invalid_slot_index\"}");
                return;
            }
            String response = ApiEndpoints::handleDeleteSignalSlot(
                nvsConfig, signalMgr, index);
            server.send(200, "application/json", response);
        });

        // POST /api/system/restart
        server.on("/api/system/restart", HTTP_POST, [this]() {
            setCorsHeaders();
            String response = ApiEndpoints::handleRestart();
            server.send(200, "application/json", response);
        });

        // Note: CORS preflight (OPTIONS) is handled by handleNotFound()

        Serial.println("[WebServer] ✓ API routes registered (9 routes)");
    }

    /**
     * Register static file serving routes
     */
    void registerStaticRoutes() {
        Serial.println("[WebServer] Registering static routes...");

        // GET / → index.html
        server.on("/", HTTP_GET, [this]() { serveStaticFile("/index.html"); });

        // GET /index.html
        server.on("/index.html", HTTP_GET,
                 [this]() { serveStaticFile("/index.html"); });

        // GET /app.js
        server.on("/app.js", HTTP_GET,
                 [this]() { serveStaticFile("/app.js"); });

        // GET /style.css
        server.on("/style.css", HTTP_GET,
                 [this]() { serveStaticFile("/style.css"); });

        // GET /favicon.ico (silent fail)
        server.on("/favicon.ico", HTTP_GET, [this]() {
            server.send(404, "text/plain", "Not Found");
        });

        Serial.println("[WebServer] ✓ Static routes registered");
    }

    /**
     * Serve static file from LittleFS
     */
    void serveStaticFile(const String& path) {
        String contentType = "text/html";

        if (path.endsWith(".js")) {
            contentType = "application/javascript";
        } else if (path.endsWith(".css")) {
            contentType = "text/css";
        } else if (path.endsWith(".json")) {
            contentType = "application/json";
        } else if (path.endsWith(".png")) {
            contentType = "image/png";
        } else if (path.endsWith(".jpg")) {
            contentType = "image/jpeg";
        }

        setCorsHeaders();

        File file = LittleFS.open(path, "r");
        if (!file) {
            Serial.printf("[WebServer] File not found: %s\n", path.c_str());
            server.send(404, "text/plain", "File Not Found");
            return;
        }

        server.streamFile(file, contentType);
        file.close();
    }

    /**
     * Handle POST /api/signals/[id]/aspect
     * Body: {"aspect":1}
     */
    void handleSignalAspectPost(const String& signalId) {
        setCorsHeaders();

        if (signalId.length() == 0) {
            server.send(400, "application/json",
                       "{\"error\":\"invalid_signal_id\"}");
            return;
        }

        if (!server.hasArg("plain")) {
            server.send(400, "application/json",
                       "{\"error\":\"missing_body\"}");
            return;
        }

        String body = server.arg("plain");
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);

        if (error) {
            server.send(400, "application/json",
                       "{\"error\":\"json_parse_failed\"}");
            return;
        }

        if (!doc.containsKey("aspect")) {
            server.send(400, "application/json",
                       "{\"error\":\"missing_aspect_field\"}");
            return;
        }

        uint8_t aspect = doc["aspect"];
        String response =
            ApiEndpoints::handleSetAspect(signalMgr, signalId, aspect);

        server.send(200, "application/json", response);
    }

    /**
     * Handle 404 and unknown requests
     */
    void handleNotFound() {
        setCorsHeaders();

        if (server.method() == HTTP_OPTIONS) {
            server.send(204);
            return;
        }

        String uri = server.uri();
        Serial.printf("[WebServer] 404: %s\n", uri.c_str());

        JsonDocument doc;
        doc["error"] = "not_found";
        doc["path"] = uri;

        String response;
        serializeJson(doc, response);

        server.send(404, "application/json", response);
    }

    /**
     * Debug: List all files in LittleFS
     */
    void listLittleFSFiles() const {
        Serial.println("[WebServer] LittleFS contents:");
        File root = LittleFS.open("/");
        File file = root.openNextFile();

        while (file) {
            Serial.printf("  ├─ %s (%u bytes)\n", file.name(), file.size());
            file = root.openNextFile();
        }
        root.close();
    }
};
