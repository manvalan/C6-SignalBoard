/*
 * SignalBoard v1.0 - I2C Hardware Abstraction Layer (HAL)
 * 
 * Thin wrapper around Wire (ESP32 I2C interface)
 * Provides dependency injection for testability and abstraction
 * 
 * Cyclomatic Complexity: ≤ 5
 * Max Method Length: 30 lines
 */

#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <cstdint>

class I2C_HAL {
public:
    explicit I2C_HAL(uint8_t sda_pin, uint8_t scl_pin) 
        : m_sda(sda_pin), m_scl(scl_pin), m_initialized(false) {}

    /**
     * Initialize I2C bus at specified frequency
     * @param frequency I2C clock frequency in Hz (default 400kHz)
     * @return true if initialization successful
     */
    bool init(uint32_t frequency = 400000) {
        Wire.begin(m_sda, m_scl, frequency);
        m_initialized = true;
        return true;
    }

    /**
     * Check if I2C device is present at given address
     * @param address 7-bit I2C address
     * @return true if device responds with ACK
     */
    bool probe(uint8_t address) const {
        if (!m_initialized) return false;
        Wire.beginTransmission(address);
        return Wire.endTransmission() == 0;
    }

    /**
     * Write single byte to I2C device register
     * @param address 7-bit I2C address
     * @param reg Register address
     * @param value Byte value to write
     * @return true if write successful
     */
    bool writeRegister(uint8_t address, uint8_t reg, uint8_t value) {
        return writeBulk(address, &reg, 1, &value, 1) >= 0;
    }

    /**
     * Read single byte from I2C device register
     * @param address 7-bit I2C address
     * @param reg Register address
     * @param out_value Pointer to output byte buffer
     * @return true if read successful
     */
    bool readRegister(uint8_t address, uint8_t reg, uint8_t* out_value) {
        return readBulk(address, &reg, 1, out_value, 1) >= 0;
    }

    /**
     * Write arbitrary data to I2C device
     * @param address 7-bit I2C address
     * @param write_buf Pointer to data to write
     * @param write_len Number of bytes to write
     * @return bytes written on success, -1 on error
     */
    int write(uint8_t address, const uint8_t* write_buf, size_t write_len) {
        if (!m_initialized || !write_buf || write_len == 0) {
            return -1;
        }
        Wire.beginTransmission(address);
        Wire.write(write_buf, write_len);
        uint8_t error = Wire.endTransmission();
        return (error == 0) ? (int)write_len : -1;
    }

    /**
     * Read arbitrary data from I2C device
     * @param address 7-bit I2C address
     * @param read_buf Pointer to input buffer
     * @param read_len Number of bytes to read
     * @return bytes read on success, -1 on error
     */
    int read(uint8_t address, uint8_t* read_buf, size_t read_len) {
        if (!m_initialized || !read_buf || read_len == 0) {
            return -1;
        }
        size_t nread = Wire.requestFrom(address, read_len);
        if (nread != read_len) return -1;
        
        for (size_t i = 0; i < read_len; i++) {
            read_buf[i] = Wire.read();
        }
        return (int)read_len;
    }

    /**
     * Write then read (common I2C pattern): send register address, then read response
     * @param address 7-bit I2C address
     * @param write_buf Register/command to write first
     * @param write_len Bytes to write
     * @param read_buf Buffer for response
     * @param read_len Bytes to read
     * @return bytes read on success, -1 on error
     */
    int writeThenRead(uint8_t address, const uint8_t* write_buf, size_t write_len,
                      uint8_t* read_buf, size_t read_len) {
        if (write(address, write_buf, write_len) < 0) {
            return -1;
        }
        delay(1);  // Small delay for device response
        return read(address, read_buf, read_len);
    }

    bool isInitialized() const { return m_initialized; }

private:
    uint8_t m_sda;
    uint8_t m_scl;
    bool m_initialized;

    // Helper: write data to register (write two phases)
    int writeBulk(uint8_t address, const uint8_t* reg_buf, size_t reg_len,
                  const uint8_t* data_buf, size_t data_len) {
        if (!m_initialized) return -1;
        Wire.beginTransmission(address);
        Wire.write(reg_buf, reg_len);
        Wire.write(data_buf, data_len);
        return Wire.endTransmission() == 0 ? (int)data_len : -1;
    }

    // Helper: read from register (read after write pattern)
    int readBulk(uint8_t address, const uint8_t* reg_buf, size_t reg_len,
                 uint8_t* data_buf, size_t data_len) {
        if (!m_initialized) return -1;
        if (write(address, reg_buf, reg_len) < 0) return -1;
        return read(address, data_buf, data_len);
    }
};
