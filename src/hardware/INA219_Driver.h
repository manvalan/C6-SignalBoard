/*
 * SignalBoard v1.0 - INA219 Power Monitor Driver
 *
 * OOP abstraction of TI INA219 bus voltage/current monitor
 * Measures the signal supply rail (bus voltage, current, power)
 *
 * Key Design:
 * - Dependency injection of I2C_HAL for testability
 * - Single responsibility: power telemetry
 * - Const-correct interface
 *
 * Registers are 16-bit big-endian:
 *   0x00 CONFIG, 0x01 SHUNT, 0x02 BUS, 0x03 POWER, 0x04 CURRENT, 0x05 CALIB
 *
 * Cyclomatic Complexity: ≤ 5
 * Max Method Length: 30 lines
 */

#pragma once

#include "../hardware/I2C_HAL.h"
#include <cstdint>

class INA219_Driver {
public:
    static constexpr uint8_t I2C_ADDR_DEFAULT = 0x44;

    enum class Register : uint8_t {
        CONFIG = 0x00,
        SHUNT_VOLTAGE = 0x01,
        BUS_VOLTAGE = 0x02,
        POWER = 0x03,
        CURRENT = 0x04,
        CALIBRATION = 0x05,
    };

    /**
     * @param hal I2C bus abstraction
     * @param address 7-bit I2C address (0x44 on SignalBoard, A0 strapped)
     * @param shunt_ohms Shunt resistor value (0.1 ohm on typical breakouts)
     * @param max_current_a Expected maximum current in Amperes
     */
    explicit INA219_Driver(I2C_HAL* hal, uint8_t address = I2C_ADDR_DEFAULT,
                           float shunt_ohms = 0.1f, float max_current_a = 3.2f)
        : m_hal(hal), m_address(address), m_initialized(false),
          m_current_lsb_ma(max_current_a * 1000.0f / 32768.0f),
          m_shunt_ohms(shunt_ohms) {}

    /**
     * Initialize INA219: verify presence, write config + calibration
     * Config 0x399F: 32V range, gain /8 (±320mV), 12-bit, continuous
     * @return true if device responds and is configured
     */
    bool init() {
        if (!m_hal || !m_hal->isInitialized()) {
            return false;
        }
        if (!m_hal->probe(m_address)) {
            return false;
        }

        if (!writeRegister16(Register::CONFIG, 0x399F)) {
            return false;
        }

        // CAL = 0.04096 / (current_LSB[A] * R_shunt[ohm])
        float current_lsb_a = m_current_lsb_ma / 1000.0f;
        uint16_t cal = (uint16_t)(0.04096f / (current_lsb_a * m_shunt_ohms));
        if (!writeRegister16(Register::CALIBRATION, cal)) {
            return false;
        }

        m_initialized = true;
        return true;
    }

    /**
     * Bus voltage in Volts (LSB = 4 mV, data in bits 15-3)
     */
    float getBusVoltage_V() const {
        uint16_t raw = 0;
        if (!readRegister16(Register::BUS_VOLTAGE, raw)) {
            return 0.0f;
        }
        return (float)(raw >> 3) * 0.004f;
    }

    /**
     * Shunt voltage in millivolts (signed, LSB = 10 uV)
     */
    float getShuntVoltage_mV() const {
        uint16_t raw = 0;
        if (!readRegister16(Register::SHUNT_VOLTAGE, raw)) {
            return 0.0f;
        }
        return (float)(int16_t)raw * 0.01f;
    }

    /**
     * Current in milliamperes (signed, LSB derived from calibration)
     */
    float getCurrent_mA() const {
        uint16_t raw = 0;
        if (!readRegister16(Register::CURRENT, raw)) {
            return 0.0f;
        }
        return (float)(int16_t)raw * m_current_lsb_ma;
    }

    /**
     * Power in milliwatts (LSB = 20 * current_LSB)
     */
    float getPower_mW() const {
        uint16_t raw = 0;
        if (!readRegister16(Register::POWER, raw)) {
            return 0.0f;
        }
        return (float)raw * (m_current_lsb_ma * 20.0f);
    }

    bool isInitialized() const { return m_initialized; }
    uint8_t getAddress() const { return m_address; }

private:
    I2C_HAL* m_hal;
    uint8_t m_address;
    bool m_initialized;
    float m_current_lsb_ma;
    float m_shunt_ohms;

    bool writeRegister16(Register reg, uint16_t value) {
        uint8_t buf[3] = {
            static_cast<uint8_t>(reg),
            static_cast<uint8_t>((value >> 8) & 0xFF),
            static_cast<uint8_t>(value & 0xFF),
        };
        return m_hal->write(m_address, buf, sizeof(buf)) >= 0;
    }

    bool readRegister16(Register reg, uint16_t& out_value) const {
        if (!m_initialized) {
            return false;
        }
        uint8_t reg_addr = static_cast<uint8_t>(reg);
        uint8_t buf[2] = {0, 0};
        if (m_hal->writeThenRead(m_address, &reg_addr, 1, buf, 2) < 0) {
            return false;
        }
        out_value = ((uint16_t)buf[0] << 8) | buf[1];
        return true;
    }
};
