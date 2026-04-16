# Building and Flashing

## Prerequisites

- [ESP-IDF v6.1](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/) installed and sourced
- Python 3 (for the asset packer)
- Target: ESP32-S3

## Build

```bash
# Source IDF environment (adjust path to your install)
source ~/esp/esp-idf-6.1/export.sh

cd source/fwi-receiver-fw
idf.py build
```

The build regenerates `sdkconfig` from `sdkconfig.defaults` on the first run. Do not commit the generated `sdkconfig` file — use `sdkconfig.defaults` for project-wide defaults.

## Flash

```bash
idf.py -p /dev/ttyUSB0 flash
```

Replace `/dev/ttyUSB0` with your serial port. To flash and open a monitor in one command:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

## Updating Web Assets

After editing any file under `source/fwi-receiver-fw/main/www/`, regenerate the embedded C files before building:

```bash
python3 source/tools/embeddedgen.py \
  -i source/fwi-receiver-fw/main/www \
  -o source/fwi-receiver-fw/main/assets
```

The tool packs HTML, CSS, JS, and font files into `EmbeddedFiles.c/.h` which are compiled into the firmware. Optional gzip compression per-file is supported via a compression config file passed with `-c`.

## OTA Update (over Wi-Fi)

1. Build the firmware: `idf.py build`
2. Connect to the device's Wi-Fi AP
3. Open `http://192.168.4.1/settings.html`
4. In the **Firmware Update** section, select `build/fwi-receiver-fw.bin`
5. Click **Upload**

The device flashes the inactive OTA partition and reboots automatically.

## Key Build Configuration

Configured via `source/fwi-receiver-fw/sdkconfig.defaults`:

| Option | Value | Notes |
|---|---|---|
| `CONFIG_IDF_TARGET` | `esp32s3` | |
| `CONFIG_ESPTOOLPY_FLASHSIZE_8MB` | `y` | 8 MB flash |
| `CONFIG_PARTITION_TABLE_CUSTOM` | `y` | Custom partition table |
| `CONFIG_LWIP_MAX_SOCKETS` | `16` | Maximum open sockets |
| `CONFIG_HTTPD_WS_SUPPORT` | `y` | WebSocket support in HTTP server |
| `CONFIG_FREERTOS_USE_TRACE_FACILITY` | `y` | Enables `vTaskList()` |
| `CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS` | `y` | |

## Directory Structure

```
source/
├── fwi-receiver-fw/        ESP-IDF project
│   ├── main/               Main component (firmware source + web assets)
│   ├── partitions.csv      Flash partition table
│   ├── sdkconfig.defaults  Build-time defaults (committed)
│   └── CMakeLists.txt      Project build entry
├── components/             Local ESP-IDF components
│   └── SSD1306/            OLED driver
├── esp32-components/       Shared components (git submodule)
│   ├── nvsjson/            NVS settings library
│   └── misc-formula/       Utility macros
└── tools/
    └── embeddedgen.py      Web asset packer
```
