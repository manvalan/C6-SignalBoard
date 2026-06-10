/*
 * SignalBoard v1.0 - Signal Device Object Model
 * 
 * Represents a single signal device with 3 color aspects (Red, Yellow, Green)
 * Abstracts signal logic (Main vs Shunt types)
 * 
 * Design:
 * - Lightweight value object
 * - Manages 3 PWM channels (one per color)
 * - Couples signal logic (aspects) with hardware mapping
 * 
 * Cyclomatic Complexity: ≤ 5
 * Max Method Length: 30 lines
 */

#pragma once

#include <Arduino.h>
#include "../hardware/PCA9685_Driver.h"
#include "../include/Enums.h"
#include <cstdint>
#include <array>

class SignalDevice {
public:
    explicit SignalDevice(const String& id, SignalType type, 
                         uint8_t pin_red, uint8_t pin_yellow, uint8_t pin_green,
                         PCA9685_Driver* pca_driver)
        : m_id(id), m_type(type), m_pca(pca_driver),
          m_current_aspect(SignalAspect::ASPECT_RED),
          m_brightness{4095, 4095, 4095}  // Default: full brightness
    {
        m_pins[0] = pin_red;
        m_pins[1] = pin_yellow;
        m_pins[2] = pin_green;
    }

    /**
     * Set signal aspect (color display)
     * @param aspect Target aspect
     * @return true if aspect changed
     */
    bool setAspect(SignalAspect aspect) {
        if (!m_pca || !m_pca->isInitialized()) {
            return false;
        }
        if (aspect == m_current_aspect) {
            return false;  // No change needed
        }

        allOff();  // Clear previous state

        if (m_type == SignalType::MAIN) {
            setAspectMain(aspect);
        } else if (m_type == SignalType::SHUNT) {
            setAspectShunt(aspect);
        }

        m_current_aspect = aspect;
        return true;
    }

    /**
     * Set brightness for specific color (0-4095)
     * @param color_index 0=Red, 1=Yellow, 2=Green
     * @param value PWM brightness (0-4095)
     */
    void setBrightness(uint8_t color_index, uint16_t value) {
        if (color_index >= 3) return;
        m_brightness[color_index] = (value > PCA9685_Driver::PWM_MAX) 
                                     ? PCA9685_Driver::PWM_MAX : value;
    }

    /**
     * Get current signal aspect
     */
    SignalAspect getCurrentAspect() const {
        return m_current_aspect;
    }

    /**
     * Get signal ID (Rocrail identifier)
     */
    const String& getId() const {
        return m_id;
    }

    /**
     * Get signal type
     */
    SignalType getType() const {
        return m_type;
    }

    /**
     * Get PWM pin number for specific color
     * @param color_index 0=Red, 1=Yellow, 2=Green
     */
    uint8_t getPin(uint8_t color_index) const {
        if (color_index >= 3) return 0xFF;
        return m_pins[color_index];
    }

    /**
     * Get brightness for specific color
     */
    uint16_t getBrightness(uint8_t color_index) const {
        if (color_index >= 3) return 0;
        return m_brightness[color_index];
    }

private:
    String m_id;
    SignalType m_type;
    PCA9685_Driver* m_pca;
    SignalAspect m_current_aspect;
    
    // Pin mapping: [Red, Yellow, Green]
    std::array<uint8_t, 3> m_pins;
    
    // Brightness values: [Red, Yellow, Green]
    std::array<uint16_t, 3> m_brightness;

    /**
     * Turn all color LEDs off
     */
    void allOff() {
        for (uint8_t i = 0; i < 3; i++) {
            m_pca->setPWM(m_pins[i], 0);
        }
    }

    /**
     * Set aspect for MAIN signal (3-aspect)
     * Red: ASPECT_RED
     * Green: ASPECT_GREEN
     * Yellow: ASPECT_YELLOW
     */
    void setAspectMain(SignalAspect aspect) {
        switch (aspect) {
            case SignalAspect::ASPECT_RED:
                m_pca->setPWM(m_pins[0], m_brightness[0]);  // Red ON
                break;
            case SignalAspect::ASPECT_GREEN:
                m_pca->setPWM(m_pins[2], m_brightness[2]);  // Green ON
                break;
            case SignalAspect::ASPECT_YELLOW:
                m_pca->setPWM(m_pins[1], m_brightness[1]);  // Yellow ON
                break;
            default:
                break;
        }
    }

    /**
     * Set aspect for SHUNT signal
     * Red: ASPECT_STOP
     * Green+Yellow: ASPECT_GO
     * Red+Yellow: ASPECT_OBLIQUE
     */
    void setAspectShunt(SignalAspect aspect) {
        switch (aspect) {
            case SignalAspect::ASPECT_STOP:
            case SignalAspect::ASPECT_RED:
                m_pca->setPWM(m_pins[0], m_brightness[0]);  // Red ON
                break;
            case SignalAspect::ASPECT_GO:
            case SignalAspect::ASPECT_GREEN:
                m_pca->setPWM(m_pins[1], m_brightness[1]);  // Yellow ON
                m_pca->setPWM(m_pins[2], m_brightness[2]);  // Green ON
                break;
            case SignalAspect::ASPECT_OBLIQUE:
                m_pca->setPWM(m_pins[0], m_brightness[0]);  // Red ON
                m_pca->setPWM(m_pins[1], m_brightness[1]);  // Yellow ON
                break;
            default:
                break;
        }
    }
};
