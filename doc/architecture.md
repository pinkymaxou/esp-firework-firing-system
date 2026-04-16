# Firmware Architecture

## Component Map

```
fwi-receiver-fw/
├── main/
│   ├── main.cpp            Boot sequence, Wi-Fi init
│   ├── MainApp.cpp/hpp     Core state machine and firing logic
│   ├── HWGPIO.cpp/hpp      Hardware abstraction (GPIO, I2C, PWM, LEDs)
│   ├── HWConfig.h          All GPIO and peripheral constants
│   ├── Settings.cpp/hpp    NVS-backed configuration table
│   ├── FWConfig.hpp        Firmware-level constants (SSID format, etc.)
│   ├── webserver/
│   │   ├── WebServer.cpp   HTTP server (file serving, OTA)
│   │   └── WSWebSocket.cpp WebSocket command handler
│   ├── oledui/
│   │   ├── UIManager.cpp   Menu controller, encoder input
│   │   ├── UIHome.cpp      Home screen (IP, SSID, user count)
│   │   ├── UIMenu.cpp      Navigation menu
│   │   ├── UISetting.cpp   Settings editor
│   │   ├── UITestConn.cpp  Connection test progress
│   │   ├── UIArmed.cpp     Armed/ready screen
│   │   └── UIAbout.cpp     Version info screen
│   ├── assets/
│   │   └── EmbeddedFiles.c Pre-packed web assets (HTML/CSS/JS)
│   └── www/                Web source files (packed by embeddedgen.py)
└── components/
    ├── SSD1306/            I2C OLED driver (C++ class)
    ├── nvsjson/            NVS + JSON settings library
    └── misc-formula/       Utility macros
```

## Boot Sequence

```
app_main()
  ├── nvs_flash_init()
  ├── HWGPIO::init()         GPIO, I2C, LED strip (RMT), LEDC PWM, encoder ISR
  ├── Settings::init()       Load NVS config, apply defaults
  ├── wifi_init()            SoftAP (192.168.4.1), optional STA
  ├── WebServer::init()      HTTP + WebSocket server on port 80
  └── g_app.init() → g_app.run()
```

## Main Loop (MainApp::run)

The main loop runs on the main task at 1 ms ticks:

1. Read master switch → update `is_armed`
2. If armed state changed → update LED blink rate and UI
3. Process any pending command (fire, test connections, calibrate)
4. Update RGB LED strip for all outputs
5. Fade sanity LED (200 ms armed / 450 ms disarmed)
6. Call `g_ui_mgr.runTick()` (OLED + encoder)

## State Machine

`EGeneralState` values:

| Value | Name | Description |
|-------|------|-------------|
| 0 | Idle | No operation in progress |
| 1 | FiringMasterSwitchWrongStateError | Fired while master switch off |
| 2 | FiringUnknownError | Unexpected firing error |
| 3 | Firing | Relay energised |
| 4 | FiringOK | Firing completed |
| 7 | Armed | System armed, ready to fire |
| 8 | CheckingConnection | Scanning outputs for continuity |
| 9 | CheckingConnectionOK | Scan complete, results ready |
| 10 | CheckingConnectionError | Scan failed |
| 11 | LiveCheckContinuity | Real-time continuity monitor active |
| 12 | DisarmedMasterSwitchOff | Automatically disarmed (switch moved) |

## Firing Sequence

```
ExecFire(index)
  ├── Guard: is_armed && index < OUTPUT_COUNT
  ├── Select relay area: MOSA pins → relay board 0 or 1
  ├── Enable master power relay (PWM, FirePWM% duty)
  ├── Set relay data line high
  ├── vTaskDelay(FireHoldTimeMS)
  ├── Clear relay data line
  ├── Disable master power relay
  └── Mark relay.is_fired = true
```

## Connection Test Sequence

```
StartTestConnections()
  For each output 0 → OUTPUT_COUNT-1:
    ├── Select relay area (MOSA pins)
    ├── Set data line
    ├── Wait 200 ms (capacitor charge)
    ├── Poll HWCONFIG_CONNSENSE_IN (max 1 s total)
    ├── relay.is_connected = sense result
    └── Clear data line
  → EGeneralState::CheckingConnectionOK
```

## WebSocket Command Routing

All browser ↔ firmware communication goes through a single WebSocket at `/ws`.

```
Browser  ──JSON cmd──►  WSWebSocket::handler()
                           └── handleCommand()
                                 ├── "getstatus"        → buildStatusJSON()
                                 ├── "getsettings"      → buildSettingsJSON()
                                 ├── "getsysinfo"       → buildSysInfoJSON()
                                 ├── "fire"             → g_app.ExecFire()
                                 ├── "checkconnections" → g_app.ExecTestConnections()
                                 ├── "setsettings"      → NVSJSON_ImportJSON()
                                 └── "reboot"           → esp_restart()
```

See [webinterface.md](webinterface.md) for the full JSON protocol.

## OLED UI Architecture

All screen classes inherit from `UIBase`:

```cpp
class UIBase {
    virtual void onEnter();
    virtual void onExit();
    virtual void onTick();
    virtual void onEncoderMove(BTEvent event, int32_t count);
    virtual void drawScreen();
};
```

`UIManager` owns an array of `UIBase*` instances (one per menu) and routes ticks and encoder events to the active one. Transitions happen via `g_ui_mgr.goTo(EMenu)`.

## Settings Library (nvsjson)

`nvsjson` wraps ESP-IDF NVS with typed, validated entries exported as JSON.

```
NVSJSON_Init() → NVSJSON_Load()   Load from NVS (apply defaults if missing)
NVSJSON_ExportJSON()              Serialise all entries to JSON string
NVSJSON_ImportJSON()              Deserialise and validate, write to NVS
NVSJSON_GetValueInt32(key)        Read a single int32 value
NVSJSON_GetValueString(key, ...)  Read a single string value
```

## Partition Layout

| Name | Type | Size | Purpose |
|------|------|------|---------|
| nvs | data/nvs | 16 KB | Settings storage |
| otadata | data/ota | 8 KB | OTA slot metadata |
| phy_init | data/phy | 4 KB | Wi-Fi PHY calibration |
| ota_0 | app/ota_0 | 1900 KB | Active firmware |
| ota_1 | app/ota_1 | 1900 KB | OTA update target |

The A/B scheme means a failed OTA flash leaves the running partition intact.

## Embedded Web Assets

Web files under `www/` are packed into `assets/EmbeddedFiles.c` at build time using `source/tools/embeddedgen.py`. The tool supports optional gzip compression per file. The HTTP server serves them directly from flash.

To regenerate after editing web files:

```bash
python3 source/tools/embeddedgen.py \
  -i source/fwi-receiver-fw/main/www \
  -o source/fwi-receiver-fw/main/assets
```
