/*
 * SignalBoard v1.0 - Rocrail XML Parser
 * Parse signal commands from MQTT messages
 * 
 * Input format: <sg id="sg1" state="green"/>
 * Output: Signal ID + Aspect enumeration
 */

#pragma once

#include <Arduino.h>
#include "../include/Enums.h"
#include <cstring>

class RocRailParser {
public:
    /**
     * Parse Rocrail signal command XML
     * @param xml_msg Raw message string (e.g., "<sg id=\"sg1\" state=\"green\"/>")
     * @param out_id Reference to output signal ID
     * @param out_aspect Reference to output aspect enum
     * @return true if parsing successful, false if malformed
     */
    static bool parseSignalCommand(const String& xml_msg, String& out_id,
                                    SignalAspect& out_aspect,
                                    SignalType signal_type) {
        if (!xml_msg.startsWith("<sg")) {
            return false;
        }

        out_id = extractAttribute(xml_msg, "id");
        if (out_id == "") {
            return false;
        }

        String state = extractAttribute(xml_msg, "state");
        if (state == "") {
            return false;
        }

        out_aspect = mapStateToAspect(state, signal_type);
        return true;
    }

    /**
     * Extract attribute value from XML element
     * e.g., extractAttribute("<sg id=\"sg1\" state=\"green\"/>", "id") → "sg1"
     */
    static String extractAttribute(const String& xml, const String& attr) {
        String search_pattern = attr + "=\"";
        int start = xml.indexOf(search_pattern);

        if (start == -1) {
            return "";
        }

        start += search_pattern.length();
        int end = xml.indexOf("\"", start);

        if (end == -1) {
            return "";
        }

        return xml.substring(start, end);
    }

    /**
     * Map XML state string to SignalAspect enum
     * Depends on signal type (MAIN vs SHUNT)
     */
    static SignalAspect mapStateToAspect(const String& state,
                                         SignalType type) {
        if (type == SignalType::MAIN) {
            return mapStateToAspectMain(state);
        } else {
            return mapStateToAspectShunt(state);
        }
    }

private:
    /**
     * MAIN signal aspect mapping:
     * red → ASPECT_RED
     * green → ASPECT_GREEN
     * yellow → ASPECT_YELLOW
     */
    static SignalAspect mapStateToAspectMain(const String& state) {
        if (state == "red") {
            return SignalAspect::ASPECT_RED;
        } else if (state == "green") {
            return SignalAspect::ASPECT_GREEN;
        } else if (state == "yellow") {
            return SignalAspect::ASPECT_YELLOW;
        }
        return SignalAspect::ASPECT_RED;  // Default to red on error
    }

    /**
     * SHUNT signal aspect mapping:
     * red → ASPECT_STOP
     * green → ASPECT_GO (yellow + green)
     * yellow → ASPECT_OBLIQUE (red + yellow)
     */
    static SignalAspect mapStateToAspectShunt(const String& state) {
        if (state == "red") {
            return SignalAspect::ASPECT_STOP;
        } else if (state == "green") {
            return SignalAspect::ASPECT_GO;
        } else if (state == "yellow") {
            return SignalAspect::ASPECT_OBLIQUE;
        }
        return SignalAspect::ASPECT_STOP;  // Default to stop on error
    }
};
