# Web Interface

## Pages

| URL | Page |
|-----|------|
| `http://192.168.4.1/` | Redirects to `/index.html` |
| `http://192.168.4.1/index.html` | Control page (firing grid) |
| `http://192.168.4.1/settings.html` | Settings, system info, OTA |

## Control Page

- Grid of buttons, one per output (32 by default)
- Button colour reflects output state (see table below)
- **Test** button runs a continuity scan on all outputs
- **Settings** button (top-right of navbar) navigates to settings
- Status bar shows current `EGeneralState` and a request counter

| Button colour | Output state |
|---------------|--------------|
| Dark (off) | Idle — no connection detected |
| Green | Connected — continuity present, system disarmed |
| Red (pulsing) | Connected — continuity present, system **armed** |
| Grey | Fired |

## Settings Page

- Table of all settings with inline editable inputs
- **Save** button pushes changes via WebSocket (`setsettings` command)
- A "Reboot required" warning appears when any changed setting has the reboot flag
- System info section shows chip model, firmware version, compile time, SHA256, IDF version, MAC addresses, heap usage, and IP addresses
- OTA section: choose a `.bin` file and click **Upload** to flash and reboot

## WebSocket Protocol

All dynamic communication uses a single WebSocket at `ws://192.168.4.1/ws`.

The browser connects on page load and polls every **250 ms** with `getstatus`. All messages are UTF-8 JSON.

### Commands (browser → device)

#### `getstatus`
```json
{ "cmd": "getstatus" }
```

#### `getsettings`
```json
{ "cmd": "getsettings" }
```

#### `getsysinfo`
```json
{ "cmd": "getsysinfo" }
```

#### `fire`
```json
{ "cmd": "fire", "index": 3 }
```
`index` is zero-based (0–31).

#### `checkconnections`
```json
{ "cmd": "checkconnections" }
```

#### `setsettings`
```json
{
  "cmd": "setsettings",
  "entries": [
    { "key": "FireHoldMS", "value": 500 },
    { "key": "WAP.Pass",   "value": "mypassword" }
  ]
}
```
Only the listed keys are updated. Keys not included are left unchanged.

#### `reboot`
```json
{ "cmd": "reboot" }
```

---

### Responses (device → browser)

#### `status`
```json
{
  "type": "status",
  "status": {
    "req": 42,
    "is_armed": false,
    "general_state": 0,
    "uptime_s": 120,
    "outputs": [
      { "ix": 0, "s": 3 },
      { "ix": 1, "s": 0 }
    ]
  }
}
```

`general_state` values:

| Value | Meaning |
|-------|---------|
| 0 | Idle |
| 1 | Master switch wrong state error |
| 2 | Unknown firing error |
| 3 | Firing |
| 4 | Firing OK |
| 7 | Armed and ready |
| 8 | Testing connections |
| 9 | Test connections OK |
| 10 | Test connections error |
| 11 | Live continuity check |
| 12 | Disarmed (master switch moved) |

Output `s` values:

| Value | Meaning |
|-------|---------|
| 0 | Idle |
| 1 | Enabled (being fired) |
| 2 | Fired |
| 3 | Connected (continuity detected) |

#### `settings`
```json
{
  "type": "settings",
  "entries": [
    {
      "key": "FireHoldMS",
      "value": 750,
      "info": {
        "desc": "How long to hold power (MS)",
        "default": 750,
        "min": 50,
        "max": 5000,
        "type": "int32",
        "flag_reboot": 1
      }
    }
  ]
}
```

#### `sysinfo`
```json
{
  "type": "sysinfo",
  "infos": [
    { "name": "Chip",           "value": "ESP32-S3" },
    { "name": "Firmware",       "value": "0.1.0" },
    { "name": "Compile Time",   "value": "Apr  1 2026 12:00:00" },
    { "name": "SHA256",         "value": "ABCDEF..." },
    { "name": "IDF",            "value": "v6.1-dev-..." },
    { "name": "WiFi.STA",       "value": "AA:BB:CC:DD:EE:FF" },
    { "name": "WiFi.AP",        "value": "AA:BB:CC:DD:EE:FF" },
    { "name": "WiFi.BT",        "value": "AA:BB:CC:DD:EE:FF" },
    { "name": "Memory",         "value": "234512 / 327680" },
    { "name": "WiFi (STA)",     "value": "0.0.0.0" },
    { "name": "WiFi (Soft-AP)", "value": "192.168.4.1" }
  ]
}
```

## OTA Upload

Firmware upload is a plain HTTP POST (not WebSocket):

```
POST /ota/upload
Content-Type: application/octet-stream
Body: raw .bin file
```

The server writes the binary to the inactive OTA partition and reboots on success. On failure it returns HTTP 500.
