# ESP32 LED Strip Controller

Firmware for ESP32 boards that drive WS2812B / WS2811 addressable LED strips, controlled over **MQTT with Home Assistant auto-discovery**. On boot the device announces itself to Home Assistant as a `light` entity (with effects) plus a `select` entity for color palettes — no manual entity configuration required. Set a solid color, run an animation, and pick the palette the animation draws from, all from the Home Assistant UI.

## Features
- **Home Assistant MQTT discovery** — the device registers itself automatically; no YAML.
- **Solid color mode** — full hue/saturation control with smooth color blending.
- **Animations** — multiple built-in effects, selectable from the light's effect dropdown.
- **Palettes** — a separate dropdown selects the color palette animations use.
- **Smooth brightness fading** between levels and on/off.
- **Per-device builds** — one codebase, multiple boards (number of strips, pins, LED counts) selected at compile time.

## Hardware Requirements
- **ESP32** — NodeMCU-32S or any other ESP32-based board.
- **LED Strips** — addressable LEDs compatible with ESP32 GPIO (WS2812B, WS2811, etc.).
- **Power Supply** — correct voltage and enough current for the total LED count.
- **MQTT broker** — e.g. the Mosquitto add-on in Home Assistant.

## Software Requirements
- **PlatformIO** — build/upload toolchain.
- **Arduino Core for ESP32** — installed automatically by PlatformIO.

Library dependencies (declared in `platformio.ini`, fetched automatically): FastLED, ArduinoJson, PubSubClient.

## Setup

1. Clone this repository:
   ```
   git clone https://github.com/mladBits/ESP32-LED.git
   ```
2. Open the project in PlatformIO.
3. Create the two local config headers (they are gitignored and **required to compile**):

   `src/config/WifiCreds.h`
   ```cpp
   #pragma once
   #define WIFI_SSID     "your-ssid"
   #define WIFI_PASSWORD "your-password"
   #define OTA_PASSWORD  "choose-an-ota-password"
   ```

   `src/config/Mqtt.h`
   ```cpp
   #pragma once
   #define MQTT_SERVER   "192.168.1.10"   // broker IP/hostname
   #define MQTT_PORT     1883
   #define MQTT_USER     "mqtt-username"
   #define MQTT_PASSWORD "mqtt-password"
   ```
4. Pick the build environment for your board and upload (see below).

Once flashed and connected, the device appears automatically in **Home Assistant → Settings → Devices & Services → MQTT**.

## Build & Upload

There is no default environment — always pass `-e <env>`. Each environment maps to a physical device and sets its pins / LED counts.

```bash
pio run -e desk_esp32                       # compile
pio run -e desk_esp32 -t upload             # compile + flash over USB
pio run -e desk_esp32 -t upload -t monitor  # flash, then open serial monitor (115200 baud)
```

| Environment   | Strips | Notes                          |
|---------------|--------|--------------------------------|
| `desk_esp32`  | 1      | WS2812B                        |
| `wall_esp32`  | 1      | WS2811                         |
| `peg_esp32`   | 3      | WS2812B                        |
| `test_esp32`  | 1      | WS2812B, bench testing         |

To add a board, add an `[env:...]` block in `platformio.ini` (with its `-DDEVICE_ID_ESP32_*`, `LED_PIN_*`, and `NUM_LEDS_*` flags) and a matching branch in `src/config/Config.h`, `src/mqtt/MqttTopics.h`, and `src/main.cpp`.

### OTA updates (upload over WiFi)

Each device also has an `*_ota` environment that uploads over WiFi instead of USB:

```bash
pio run -e desk_esp32_ota -t upload
```

- The **first flash of a device must be over USB** (the OTA code has to be on the board before it can receive updates). After that, OTA works from anywhere on the LAN.
- Uploads are authenticated with the `OTA_PASSWORD` from `WifiCreds.h` — the build reads it automatically (`scripts/ota_auth.py`), nothing extra to pass.
- The device is addressed as `<mqtt_client_id>.local` (e.g. `esp32_desk_led.local`). If mDNS doesn't resolve on your machine (common on Windows without Bonjour), pass the device IP instead:

  ```bash
  pio run -e desk_esp32_ota -t upload --upload-port 192.168.1.42
  ```
- The LEDs blank during the transfer and the device reboots into the new firmware.
- If the device is mid MQTT-reconnect (broker down), it can be unresponsive to OTA for up to ~30 s — just retry.

## Control

All control is via MQTT. In normal use you interact through the Home Assistant UI — the light card for power/brightness/color/effect, and the **Palette** dropdown for the color palette. The topics below are documented for reference and manual testing. The base prefix is `homeassistant/light/<device>` (e.g. `homeassistant/light/esp32_desk_led`).

### Light — `…/set` (JSON)
Power, brightness, color, and effect, using the Home Assistant JSON light schema:
```json
{ "state": "ON", "brightness": 180, "color": { "h": 210, "s": 80 }, "effect": "Pacifica" }
```
- `state` — `"ON"` / `"OFF"`
- `brightness` — `0`–`255`
- `color` — `h` in degrees (0–360), `s` in percent (0–100)
- `effect` — one of the effect names below (switches to animation mode)
- `effect_dir` — `"Outward"` / `"Inward"` (custom extension, not exposed in the HA UI)

Sending a `color` switches back to solid-color mode. Current state is published to `…/state`.

### Palette — `…/palette/set1` (JSON)
```json
{ "palette": "OceanColors" }
```
Home Assistant's auto-generated **Palette** select sends this for you. The available options are published to `…/palette/list`, and the current selection to `…/palette/state`.

### Built-in effects
`Plasma`, `ColorWave`, `Pacifica`, `Cyber Pulse`, `Neon Grid Breathing`, `Ember Bloom`

### Palettes
A range of FastLED built-ins (RainbowColors, OceanColors, LavaColors, …) plus custom palettes (SunsetColors, Cyberpunk, ToxicSlime, NeonStorm, and more). The full, current list is whatever the device publishes to `…/palette/list`.

## Notes
- Brightness is applied globally via FastLED, not as the color's value channel, so colors stay true across brightness levels.
- Discovery configs are published retained on every (re)connect. If you change unique IDs or see stale entities, remove the device under the MQTT integration and let it re-announce.
