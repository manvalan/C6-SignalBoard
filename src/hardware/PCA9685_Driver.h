/*
 * SignalBoard v1.0 - PCA9685 PWM Driver
 * 
 * OOP abstraction of 16-channel PWM controller
 * Manages frequency, duty cycle, and power states
 * 
 * Key Design:
 * - Dependency injection of I2C_HAL for testability
 * - Single responsibility: PWM channel control
 * - Const-correct interface
 * 
 * Cyclomatic Complexity: ≤ 5
 * Max Method Length: 30 lines
 */

#pragma once

#include "../hardware/I2C_HAL.h"
#include <cstdint>
#include <cstring>

class PCA9685_Driver {
public:
    static constexpr uint8_t CHANNELS = 16;
    static constexpr uint16_t PWM_MAX = 4095;
    static constexpr uint8_t I2C_ADDR_BASE = 0x40;

    // PCA9685 Register Map
    enum class Register : uint8_t {
        MODE1 = 0x00,
        MODE2 = 0x01,
        LED0_ON_L = 0x06,
        LED0_ON_H = 0x07,
        LED0_OFF_L = 0x08,
        LED0_OFF_H = 0x09,
        // LED1-LED15 follow same pattern with +4 offset
        PRE_SCALE = 0xFE,
    };

    explicit PCA9685_Driver(I2C_HAL* hal, uint8_t address = I2C_ADDR_BASE)
        : m_hal(hal), m_address(address), m_initialized(false), m_frequency(50) {
        std::memset(m_pwm_values, 0, sizeof(m_pwm_values));
    }

    /**
     * Initialize PCA9685 and verify communication
     * @return true if device responds
     */
    bool init() {
        if (!m_hal || !m_hal->isInitialized()) {
            return false;
        }
        if (!m_hal->probe(m_address)) {
            return false;
        }
        
        uint8_t mode1 = 0;
        if (!m_hal->readRegister(m_address, static_cast<uint8_t>(Register::MODE1), &mode1)) {
            return false;
        }
        
        m_initialized = true;
        setFrequency(1000);  // Default: 1 kHz for LED driver
        wakeup();
        return true;
    }

    /**
     * Set PWM frequency (16 Hz to 1526 Hz)
     * Formula: PRE_SCALE = (25 MHz / (4096 * frequency)) - 1
     * @param frequency_hz Target frequency in Hz
     */
    void setFrequency(uint16_t frequency_hz) {
        if (!m_initialized || frequency_hz < 16 || frequency_hz > 1526) {
            return;
        }

        m_frequency = frequency_hz;
        uint16_t prescale = (25000000 / (4096 * frequency_hz)) - 1;
        prescale = (prescale > 255) ? 255 : prescale;

        sleep();
        m_hal->writeRegister(m_address, static_cast<uint8_t>(Register::PRE_SCALE), 
                            static_cast<uint8_t>(prescale));
        wakeup();
    }

    /**
     * Set PWM value for single channel
     * @param channel Channel number (0-15)
     * @param value PWM value (0-4095)
     */
    void setPWM(uint8_t channel, uint16_t value) {
        if (!m_initialized || channel >= CHANNELS || value > PWM_MAX) {
            return;
        }

        m_pwm_values[channel] = value;
        
        // Channel ON/OFF register offsets: each channel is 4 bytes apart
        uint8_t led_on_l = static_cast<uint8_t>(Register::LED0_ON_L) + (channel << 2);
        uint8_t led_off_l = static_cast<uint8_t>(Register::LED0_OFF_L) + (channel << 2);

        // Full ON or proportional PWM
        if (value == PWM_MAX) {
            m_hal->writeRegister(m_address, led_on_l, 0x10);  // Full ON (bit 4)
        } else if (value == 0) {
            m_hal->writeRegister(m_address, led_off_l, 0x10); // Full OFF (bit 4)
        } else {
            m_hal->writeRegister(m_address, led_on_l, 0x00);  // ON at 0
            m_hal->writeRegister(m_address, led_off_l, static_cast<uint8_t>(value & 0xFF));
            m_hal->writeRegister(m_address, static_cast<uint8_t>(led_off_l + 1), 
                                static_cast<uint8_t>((value >> 8) & 0x0F));
        }
    }

    /**
     * Get last known PWM value for channel
     * @param channel Channel number (0-15)
     * @return PWM value (0-4095) or 0 if invalid
     */
    uint16_t getPWM(uint8_t channel) const {
        if (channel >= CHANNELS) return 0;
        return m_pwm_values[channel];
    }

    /**
     * Power down device (SLEEP mode)
     */
    void sleep() {
        if (!m_initialized) return;
        uint8_t mode1 = 0;
        m_hal->readRegister(m_address, static_cast<uint8_t>(Register::MODE1), &mode1);
        mode1 |= 0x10;  // Set SLEEP bit
        m_hal->writeRegister(m_address, static_cast<uint8_t>(Register::MODE1), mode1);
        delay(1);
    }

    /**
     * Power up device (WAKE mode)
     */
    void wakeup() {
        if (!m_initialized) return;
        uint8_t mode1 = 0;
        m_hal->readRegister(m_address, static_cast<uint8_t>(Register::MODE1), &mode1);
        mode1 &= ~0x10;  // Clear SLEEP bit
        m_hal->writeRegister(m_address, static_cast<uint8_t>(Register::MODE1), mode1);
        delay(1);
    }

    /**
     * Turn all channels OFF immediately
     */
    void allOff() {
        if (!m_initialized) return;
        for (uint8_t ch = 0; ch < CHANNELS; ch++) {
            setPWM(ch, 0);
        }
    }

    bool isInitialized() const { return m_initialized; }
    uint16_t getFrequency() const { return m_frequency; }
    uint8_t getAddress() const { return m_address; }

private:
    I2C_HAL* m_hal;
    uint8_t m_address;
    bool m_initialized;
    uint16_t m_frequency;
    uint16_t m_pwm_values[CHANNELS];
};
