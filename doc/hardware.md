# Hardware Reference

## GPIO Pinout

| GPIO | Direction | Function |
|------|-----------|----------|
| 0 | Output | Sanity LED (PWM, LEDC ch1, active-low) |
| 1 | Output | Relay bus line 16 |
| 2 | Output | Relay bus line 8 |
| 4 | Output | Master power relay (PWM, LEDC ch0, 200 Hz) |
| 5 | Output | Relay bus line 9 |
| 6 | Output | Relay bus line 1 |
| 7 | Output | Relay bus line 2 |
| 8 | Output | Relay bus line 7 |
| 9 | Output | Relay bus line 10 |
| 10 | Output | Relay bus line 11 |
| 11 | Output | Relay bus line 12 |
| 12 | Output | Relay bus line 13 |
| 13 | Output | Relay bus line 14 |
| 14 | Output | Relay bus line 15 |
| 15 | Output | Relay bus line 3 |
| 16 | Output | Relay bus line 4 |
| 17 | Output | Relay bus line 5 |
| 18 | Output | Relay bus line 6 |
| 21 | Output | I2C SCL |
| 37 | Input | Connection sense (continuity detect, active-high) |
| 38 | Input | Encoder push button (active-low) |
| 39 | Input | Master power sense / safety switch (active-low) |
| 40 | Output | Relay area select MOSA1 |
| 41 | Output | Relay area select MOSA2 |
| 42 | Output | Relay area select MOSA3 (reserved, area 2 unused) |
| 43 | Input | Encoder channel A |
| 44 | Input | Encoder channel B |
| 47 | Input/Output | I2C SDA *(see errata)* |
| 48 | Output | WS2812B LED ring (RMT) *(see errata)* |

## Relay Bus Architecture

Relay boards share a 16-line data bus. A pair of MOSA select lines choose which board (area) is active.

```
Data bus (GPIOs 1,2,5–18): 16 lines  ─┬─► Relay board 0  (outputs  0–15)  MOSA1=1, MOSA2=0
                                        ├─► Relay board 1  (outputs 16–31)  MOSA1=0, MOSA2=1
                                        └─► Relay board 2  (outputs 32–47)  MOSA3=1 [reserved]
```

Only one area may be selected at a time. The bus is de-energised between operations with a 25 ms settling delay.

**Current configuration**: `HWCONFIG_OUTPUTAREA_COUNT = 2` → 32 outputs.  
To enable 48 outputs, change the constant to 3 and connect relay board 2 to MOSA3 (GPIO 42).

## I2C Bus

- Speed: 100 kHz
- SDA: GPIO 47
- SCL: GPIO 21
- OLED (SSD1306): address `0x3C`

## Power

- Input voltage: 12–18 V
- Master power relay driven via PWM (200 Hz, duty configurable 5–100 %)
- Default PWM duty: 25 % (reduces current stress on the supply)

## LED Ring

The WS2812B ring on GPIO 48 provides visual feedback:

| LED index | Meaning |
|-----------|---------|
| 0 | Sanity indicator — green: disarmed, red: armed |
| 1–32 | Per-output state — see colour map below |

| Output state | Colour |
|---|---|
| Idle | Off |
| Connected (disarmed) | Yellow |
| Connected (armed) | Red (pulsing) |
| Fired | Grey |

## PCB Errata

### v1.0

1. **GPIO 0 sanity LED** — 3.3 V driven instead of ground driven, creating a pull-down that prevents booting. Leave unconnected.
2. **SDA / LED ring swapped** — GPIO 47 and 48 are swapped on the PCB. Correct by swapping the physical wires:
   - GPIO 47 → LED ring (WS2812B data)
   - GPIO 48 → SDA (I2C)

### v1.1

Both issues above are fixed.
