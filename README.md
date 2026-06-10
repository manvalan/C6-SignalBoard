# SignalBoard v1.0 - ESP32 Railway Signal Controller

**Professional embedded firmware** for railway signal control via PCA9685 PWM driver and Rocrail MQTT integration.

## 📋 Project Status: Phase 4 🚧 In Progress

| Phase | Scope | Status |
|-------|-------|--------|
| 1 | Hardware foundation (I2C HAL, PCA9685 driver, SignalDevice) | ✅ Done |
| 2 | Logic layer (NVS config, SignalManager, MQTT/Rocrail) | ✅ Done |
| 3 | Web server, REST API, dashboard + signal configuration UI | ✅ Done |
| 4a | ESP32-C6 target (pioarduino, Arduino core 3.3.7) | ✅ Done |
| 4b | Web authentication (Basic auth on config/OTA/restart) | ✅ Done |
| 4c | OTA updates (firmware + filesystem via web UI) | ✅ Done |
| 4d | INA219 power monitoring, hardware bring-up, unit tests | 🔜 Next |

### What's Included (Phase 1 - Foundation)

#### Architecture
- **Hardware Abstraction Layer (HAL)**: I2C bus abstraction with dependency injection
- **PCA9685 OOP Driver**: Full 16-channel PWM controller with frequency/brightness management
- **Signal Device Model**: Lightweight object model for 5 signal devices (Main and Shunt types)
- **Modular Structure**: `src/hardware/`, `src/app/`, `src/ui/` directories ready for Phase 2

#### Code Quality
- ✅ **Cyclomatic Complexity** ≤ 5 (flat, readable logic)
- ✅ **Method Length** ≤ 30 lines (focused functions)
- ✅ **Const-Correctness** enforced throughout
- ✅ **RAII Pattern** applied to resource management
- ✅ **C++2a (C++20 equivalent)** enabled

#### Compilation Status
```
Production:  esp32-c6-devkitc-1 (pioarduino 55.03.37, Arduino core 3.3.7, IDF 5.5.2)
             Flash 35.8% (1.2 MB / 3.3 MB, OTA layout 8 MB), RAM 14.3%
Test board:  esp32dev-test (espressif32, Arduino core 2.x)
Build:       SUCCESS on both environments
```

---

## 📁 Project Structure

```
C6-SignalBoard/
├── platformio.ini                    # PlatformIO config + dependencies
├── include/
│   ├── Pins.h                        # ESP32-C6 pin definitions
│   ├── Config.h                      # Global config constants
│   └── Version.h                     # Firmware version info
├── src/
│   ├── main.cpp                      # Entry point (setup + loop)
│   ├── hardware/
│   │   ├── I2C_HAL.h                 # I2C bus abstraction
│   │   ├── PCA9685_Driver.h          # 16-ch PWM controller (OOP)
│   │   └── SignalDevice.h            # Signal device model (5 max)
│   ├── app/                          # Phase 2: Logic layer
│   └── ui/                           # Phase 2: Web/MQTT layer
├── data/                             # Phase 3: LittleFS web UI
├── doc/                              # C6-SignalBoard.pdf schema
├── .git/                             # Version control
└── README.md                         # This file
```

---

## 🔌 Hardware Configuration (ESP32-C6 Target)

### I2C Bus
- **SDA Pin**: GPIO 8
- **SCL Pin**: GPIO 9
- **Frequency**: 400 kHz standard

### PCA9685 (16-ch PWM Driver)
- **I2C Address**: 0x40 (configurable to 0x40-0x49 for multi-driver support)
- **Signal Mapping**: 5 signals × 3 channels each = 15 PWM channels (ch. 15 reserved)
- **PWM Frequency**: 1000 Hz (LED driver optimized)
- **Brightness**: 0-4095 (12-bit resolution)

### Signal Devices
| Signal | Red | Yellow | Green |
|--------|-----|--------|-------|
| SIG 1  | CH0 | CH1    | CH2   |
| SIG 2  | CH3 | CH4    | CH5   |
| SIG 3  | CH6 | CH7    | CH8   |
| SIG 4  | CH9 | CH10   | CH11  |
| SIG 5  | CH12| CH13   | CH14  |

### GPIO Control
- **Status LED**: GPIO 2 (blue indicator)
- **Reset Button**: GPIO 1 (factory reset, optional)
- **Power Monitoring**: I2C to INA219 at 0x44 (optional)

---

## 🎯 Key Design Decisions

### 1. HAL Pattern (I2C_HAL.h)
Thin wrapper around Wire library enables:
- Unit testing without hardware
- Easy migration to ESP-IDF backend
- Multiple I2C instances support

### 2. OOP Driver (PCA9685_Driver.h)
- Single responsibility: PWM channel management
- Encapsulates 16 channels internally
- Frequency scaling (16-1526 Hz)
- Full ON/OFF optimization for LED drivers

### 3. SignalDevice Model
- Couples hardware pins with signal logic
- Supports MAIN (3-aspect) and SHUNT (3-aspect with oblique) types
- Per-color brightness customization
- No Rocrail knowledge (decoupled logic)

### 4. Five-Signal Support
- Dynamic allocation ready for Phase 2 (`std::vector<SignalDevice>`)
- NVS configuration keys prepared (legacy compatible)
- Future MQTT dispatch will use signal ID → device mapping

---

## 🚀 Compilation & Build

### Prerequisites
```bash
pip3 install platformio intelhex
```

### Build Firmware
```bash
cd C6-SignalBoard
python3 -m platformio run                        # ESP32-C6 (default env)
python3 -m platformio run -e esp32dev-test       # legacy ESP32 test board
python3 -m platformio run -t buildfs             # LittleFS web UI image
```

> Note: the pioarduino builder requires `littlefs-python` and `fatfs-ng`
> in the PlatformIO Python environment (`pip3 install littlefs-python fatfs-ng`).

### Build Output
- Binary: `.pio/build/esp32-c6-devkitc-1/firmware.bin`
- Web UI: `.pio/build/esp32-c6-devkitc-1/littlefs.bin`

### Upload to Hardware
```bash
python3 -m platformio run -t upload --upload-port=/dev/ttyUSB0
python3 -m platformio run -t uploadfs --upload-port=/dev/ttyUSB0
```

### OTA Update (after first flash)
From the dashboard ("Firmware Update" section) or via curl:
```bash
curl -u admin:signal -F "update=@firmware.bin" http://signal.local/api/ota/firmware
curl -u admin:signal -F "update=@littlefs.bin" http://signal.local/api/ota/filesystem
```
Configuration, restart and OTA endpoints require Basic auth
(default `admin` / `signal`, password stored in NVS key `web_pass`).

---

## 📝 API Reference (Phase 1)

### I2C_HAL
```cpp
bool init(uint32_t frequency = 400000);
bool probe(uint8_t address);
bool writeRegister(uint8_t address, uint8_t reg, uint8_t value);
bool readRegister(uint8_t address, uint8_t reg, uint8_t* out_value);
int write(uint8_t address, const uint8_t* buf, size_t len);
int read(uint8_t address, uint8_t* buf, size_t len);
int writeThenRead(uint8_t addr, const uint8_t* wbuf, size_t wlen,
                  uint8_t* rbuf, size_t rlen);
```

### PCA9685_Driver
```cpp
bool init();
void setFrequency(uint16_t freq_hz);
void setPWM(uint8_t channel, uint16_t value);  // 0-4095
uint16_t getPWM(uint8_t channel) const;
void sleep();
void wakeup();
void allOff();
```

### SignalDevice
```cpp
bool setAspect(SignalAspect aspect);
void setBrightness(uint8_t color_idx, uint16_t brightness);
SignalAspect getCurrentAspect() const;
const String& getId() const;
SignalType getType() const;
uint8_t getPin(uint8_t color_idx) const;
uint16_t getBrightness(uint8_t color_idx) const;
```

---

## ✅ Implemented (Phases 2-3)

- **NVS Configuration**: Load/save 5 signals from ESP32 Preferences (`network` + `railway` namespaces)
- **MQTT Interface**: Subscribe to Rocrail `rocrail/service/info/sg`, LWT on `railway/status/segnali`, feedback publish
- **RocRail Parser**: XML message parsing + aspect dispatch (MAIN/SHUNT mapping)
- **Web Server**: REST API + static dashboard from LittleFS, CORS support
- **REST API**:
  - `GET /api/signals`, `GET /api/signals/{id}`, `GET /api/signals/status`
  - `POST /api/signals/{id}/aspect` — set aspect from browser
  - `GET /api/config`, `POST /api/config/network`
  - `GET /api/config/signals`, `POST/DELETE /api/config/signals/{index}` — full signal slot CRUD with live reload (no restart)
  - `GET /api/system`, `POST /api/system/restart`
- **Web Dashboard** (`data/`): live signal control, system stats, signal slot configuration UI (ID, type, PCA9685 channels, per-color brightness)

## ✅ Implemented (Phase 4a-c)

- **ESP32-C6 target**: `esp32-c6-devkitc-1` env via pioarduino (Arduino core 3.3.7 / IDF 5.5.2), 8 MB OTA partition layout, zero code changes required
- **Web Authentication**: Basic auth on `POST /api/config/*`, `POST/DELETE /api/config/signals/*`, `POST /api/system/restart` and OTA endpoints
- **OTA Updates**: `POST /api/ota/firmware` + `POST /api/ota/filesystem` (multipart upload, dashboard UI with progress bar, auto-reboot)

## 🔮 Remaining (Phase 4d)

- **INA219 Power Monitoring**: bus voltage/current on the dashboard
- **Hardware validation**: I2C/PCA9685 bring-up on the physical SignalBoard
- **Unit tests**: native tests for RocRailParser, SignalDevice aspect logic (test/ folder is ready)

---

## 📚 References

- [ESP32 Arduino Framework](https://docs.espressif.com/projects/arduino-esp32/en/latest/)
- [PCA9685 Datasheet](https://www.nxp.com/docs/en/data-sheet/PCA9685.pdf)
- [Rocrail Protocol](https://wiki.rocrail.net/)
- [C++ Code That Fits in Your Head](https://www.infoq.com/presentations/focus-reading-code/)

---

## 📄 License

MIT License - See LICENSE file (if included)

---

## ✍️ Author

**Michele Bigi** – Senior Embedded Software Engineer

**Built with:**
- C++20 (C++2a equivalent)
- Arduino Framework for ESP32
- PlatformIO

**Last Updated:** June 10, 2026
