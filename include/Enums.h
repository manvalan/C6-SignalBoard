/*
 * SignalBoard v1.0 - Shared Enumerations
 * Common types used across hardware, app, and UI layers
 */

#pragma once

#include <cstdint>

// ===== Signal Type =====
enum class SignalType : uint8_t {
    MAIN = 0,    // Main line signal (3-aspect: Red, Green, Yellow)
    SHUNT = 1    // Shunt/Marmotta signal (Red, Green, Yellow+Red)
};

// ===== Signal Aspect =====
enum class SignalAspect : uint8_t {
    ASPECT_RED = 0,       // Red/Stop (both MAIN & SHUNT)
    ASPECT_GREEN = 1,     // Green (MAIN only)
    ASPECT_YELLOW = 2,    // Yellow (MAIN only)
    ASPECT_STOP = 3,      // Alias for RED (SHUNT)
    ASPECT_GO = 4,        // Green + Yellow (SHUNT)
    ASPECT_OBLIQUE = 5    // Red + Yellow (SHUNT)
};

// ===== MQTT Connection State =====
enum class MqttState : uint8_t {
    DISCONNECTED = 0,
    CONNECTING = 1,
    CONNECTED = 2,
    ERROR = 3
};

// ===== Signal State Names for XML/JSON =====
const char* SIGNAL_STATE_NAMES[] = {
    "red",
    "green",
    "yellow",
    "stop",
    "go",
    "oblique"
};
