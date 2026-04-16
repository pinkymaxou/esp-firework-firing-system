# Settings Reference

Settings are stored in the NVS partition and survive reboots. They can be edited via:
- The **Settings** web page (inline editing + Save button)
- The **OLED encoder menu** (Setting screen)
- The WebSocket `setsettings` command directly

All settings marked **Reboot required** take effect after the next reboot.

## Setting Entries

| Key | Type | Default | Range | Reboot | Description |
|---|---|---|---|---|---|
| `espn.pmk` | string | `pmk1234567890123` | — | yes | ESP-NOW primary master key (16 bytes) |
| `WiFi.Chan` | int32 | `3` | 1–11 | yes | Wi-Fi channel. The SoftAP and any connected station must use the same channel |
| `WAP.Pass` | string | *(empty)* | empty or >8 chars | yes | SoftAP password. Empty = open network |
| `WSTA.IsActive` | int32 | `0` | 0–1 | yes | Enable Wi-Fi station mode (connect to an existing AP) |
| `WSTA.SSID` | string | *(empty)* | — | yes | Station mode SSID to connect to |
| `WSTA.Pass` | string | *(empty)* | empty or >8 chars | yes | Station mode password |
| `FireHoldMS` | int32 | `750` | 50–5000 | yes | Duration in milliseconds the relay is held energised during firing |
| `FirePWM` | int32 | `25` | 5–100 | yes | Master power relay PWM duty cycle (%). Lower values reduce current draw |

## Notes

### Wi-Fi AP Password
The `WAP.Pass` validator requires either an empty string (open network) or a string longer than 8 characters. WPA2-PSK is used when a password is set.

### Wi-Fi Channel
ESP-NOW and the SoftAP share the same channel. If station mode (`WSTA.IsActive = 1`) is enabled, the channel is determined by the upstream AP being connected to. Set `WiFi.Chan` to match.

### Firing Parameters
- `FireHoldMS` controls how long the igniter is energised. Too short may not ignite; too long wastes power and heats the relay.
- `FirePWM` sets the duty cycle of the master power MOSFET. At 25 % the relay board receives reduced average voltage, limiting inrush current. Increase if igniters fail to fire reliably.

### Station Mode
When `WSTA.IsActive = 1`, the firmware attempts to join the configured SSID. The device remains accessible via SoftAP at `192.168.4.1` regardless of station connection status. The STA IP is shown on the OLED home screen and in the system info page.
