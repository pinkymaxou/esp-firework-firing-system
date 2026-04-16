# ESP Firework Firing System

[![Build](https://github.com/pinkymaxou/esp-firework-firing-system/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/pinkymaxou/esp-firework-firing-system/actions/workflows/build.yml)

An ESP32-S3 based remote firework ignition controller. Up to 32 outputs (expandable to 48) can be fired individually via a Wi-Fi web interface or the onboard OLED + rotary encoder.

![Schematic v2.1](./doc/assets/Schematic_esp-firework-firing-system-v2.1.png)

## Features

- **32 outputs** (expandable to 48 with a third relay board)
- **Per-output RGB LED** status indicator (continuity / armed / fired)
- **Continuity check** before firing
- **Hardware safety switch** — physical arming required before any output can fire
- **12–18 V** input power, configurable PWM firing duty and hold time
- **Wi-Fi soft-AP** (and optional STA mode) — no router required
- **Embedded HTTP + WebSocket server** — no cloud, no app, just a browser
- **OLED display** (SSD1306 128×64) with rotary encoder menu
- **OTA firmware update** via the web interface

## Hardware Requirements

| Component | Details |
|-----------|---------|
| MCU | ESP32-S3 |
| Relay boards | Up to 3 × 16-channel relay board |
| Display | SSD1306 128×64 OLED (I2C, optional) |
| Input | Rotary encoder with push button (optional) |
| Power | 12–18 V DC |

PCB design files and schematic are in [`cad/`](cad/).

## Quick Start

1. **Flash** the firmware — see [doc/building.md](doc/building.md)
2. **Connect** to Wi-Fi AP **FWI-XXYYZZ** (last 3 bytes of MAC address, no password by default)
3. **Open** `http://192.168.4.1` in a browser
4. **Arm** by toggling the physical master switch
5. **Fire** by clicking any green output button

## Building

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/pinkymaxou/esp-firework-firing-system.git

# Source ESP-IDF v6.1
source ~/esp/esp-idf-6.1/export.sh

# Build
cd source/fwi-receiver-fw
idf.py build

# Flash
idf.py -p /dev/ttyUSB0 flash monitor
```

See [doc/building.md](doc/building.md) for full instructions including OTA updates.

## Documentation

| Document | Description |
|----------|-------------|
| [doc/hardware.md](doc/hardware.md) | GPIO pinout, relay bus architecture, PCB errata |
| [doc/architecture.md](doc/architecture.md) | Firmware architecture and component overview |
| [doc/webinterface.md](doc/webinterface.md) | WebSocket protocol and web UI |
| [doc/settings.md](doc/settings.md) | Configurable settings reference |
| [doc/building.md](doc/building.md) | Build, flash, and OTA instructions |
| [doc/bom.md](doc/bom.md) | Bill of materials |
| [doc/coding-style.md](doc/coding-style.md) | Coding style guide |

## Known Issues

### Encoder conflicts with UART0 (all PCB versions)

The rotary encoder uses GPIOs 43 and 44, which are also UART0 TX/RX (the ESP32-S3 USB-serial programming and debug interface). With the encoder connected, serial flashing may fail and the serial monitor output will be corrupted. **Disconnect the encoder before flashing over serial.** OTA update is unaffected.

## PCB Errata — v1.0

PCB v1.0 has two errors fixed in v1.1:

- **GPIO 0 sanity LED**: On v1.0 the LED is 3.3 V-driven instead of ground-driven, creating a pull-down that prevents booting. Leave it unconnected on v1.0.
- **Pins 47/48 swapped**: On v1.0, SDA (I2C) and the WS2812B LED ring are swapped. Swap the wires:
  - GPIO 47 → LED ring
  - GPIO 48 → SDA
