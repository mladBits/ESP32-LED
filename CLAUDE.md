# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

Firmware for ESP32 boards that drive WS2812B/WS2811 addressable LED strips. Despite the README mentioning a REST API, the actual control path is **MQTT with Home Assistant discovery** — the device auto-registers as an HA `light` entity and is controlled via JSON command messages. Built with PlatformIO + Arduino framework + FastLED.

## Build / Upload / Monitor

PlatformIO is the toolchain. There are **multiple environments**, one per physical device, each selected with `-e`. There is no single "default" — always specify the environment.

```bash
pio run -e desk_esp32                    # compile
pio run -e desk_esp32 -t upload          # compile + flash over USB
pio run -e desk_esp32 -t upload -t monitor   # flash then open serial monitor
pio device monitor -b 115200             # serial monitor only (115200 baud)
pio run -t clean -e desk_esp32           # clean build artifacts
```

Environments (defined in [platformio.ini](platformio.ini)): `peg_esp32` (3 strips), `desk_esp32`, `wall_esp32`, `test_esp32`. Each sets a `-DDEVICE_ID_ESP32_*` macro plus `LED_PIN_*` / `NUM_LEDS_*` build flags. Per-device identity (MQTT client/unique IDs, names) and topic prefixes are resolved at compile time from that macro in [src/config/Config.h](src/config/Config.h) and [src/mqtt/MqttTopics.h](src/mqtt/MqttTopics.h). **Adding a board = add an env block in `platformio.ini` and a matching `#elif` branch in those two files** (and the strip-allocation block in [src/main.cpp](src/main.cpp)).

There is no test runner wired up — `test/` contains only the PlatformIO placeholder README.

## Required local config (gitignored, not in repo)

These two headers are `.gitignore`d and **must be created before the project will compile**:

- `src/config/WifiCreds.h` — defines `WIFI_SSID`, `WIFI_PASSWORD`
- `src/config/Mqtt.h` — defines `MQTT_SERVER`, `MQTT_PORT`, `MQTT_USER`, `MQTT_PASSWORD`

If a build fails on missing symbols from these, the files need creating — do not commit them.

## Architecture

The control flow is a one-directional pipeline: **MQTT message → `MqttLight` (intent/state) → `LEDController` (per-strip ops) → `Strip` (state) → `Animation` (renders into the strip's `CRGB[]`) → `FastLED.show()`**.

- **[src/main.cpp](src/main.cpp)** — allocates the `CRGB[]` arrays statically (count/pins from build flags), registers strips with FastLED and the `LEDController`, wires up MQTT, and runs the loop. Chipset/color order is chosen per device (`WALL` = WS2811/RGB, others = WS2812B/GRB).

- **[src/mqtt/MqttLight.cpp](src/mqtt/MqttLight.cpp)** — the brain. Owns all user-facing state (power, brightness, hue/sat, selected effect, blend state). `loop()` runs every tick: handles MQTT reconnect, smoothly ramps brightness toward `targetBrightness`, and either drives the selected animation or renders a blended solid color. Key design points:
  - **Brightness is applied via `FastLED.setBrightness`, never as the HSV `V` channel** — solid colors are always rendered at full V.
  - Solid color changes **interpolate** hue (shortest path around the 255-wrap circle) and saturation over `solidBlendDurationMs` (~1s).
  - `callback()` parses HA JSON commands on `SET_TOPIC` (state/brightness/color/effect/`effect_dir`) and palette selections on `PALETTE_SET_TOPIC`.
  - On (re)connect it publishes HA discovery config (`publishDeviceConfig`) and the palette list, then subscribes.

- **[src/LEDController.cpp](src/LEDController.cpp)** — thin fan-out layer. Holds up to `MAX_STRIPS` (8) `Strip`s and applies every operation (clear, fill solid, animate, set palette/direction) across all of them. Stateless beyond the strip array + `isOn`.

- **[src/Strip.h](src/Strip.h)** — per-strip state struct: `leds` pointer + count, `currentPalette`/`targetPalette` (animations blend current toward target), `direction`, and the active `Animation*`.

- **Animations** ([src/animation/](src/animation/)) — each implements the `Animation` interface (`update(Strip&)` + `getName()`). They are **stateless singletons**: instantiated once in [AnimationRegistry.cpp](src/animation/AnimationRegistry.cpp), looked up by name, and write directly into `strip.leds`. The registry's `list()`/`getByName()` also feed the HA `effect_list`. **To add an animation: create the class, add the member + `animations[count++]` line in the registry constructor.** (Note: `CyberPulse.h` pairs with the misspelled `CyperPulse.cpp`.)

- **[src/PaletteManager.cpp](src/PaletteManager.cpp)** — name→`CRGBPalette16` map; serializes the available palette names to JSON for the `palette/list` topic and resolves incoming palette selections.

- **[src/HomeAssistantColor.h](src/HomeAssistantColor.h)** — conversions between HA's hue-degrees/sat-percent and FastLED's 0–255 hue/sat. Anywhere color crosses the MQTT boundary, it goes through these.

- **[src/NetworkManager.h](src/NetworkManager.h)** — static WiFi connect helper (STA mode, retry loop). `loop()` reconnects if WiFi drops.

## Conventions

- Device-specific behavior is handled with **compile-time `#if defined(DEVICE_ID_ESP32_*)` branches**, not runtime config. When touching anything device-specific, update every branch.
- Animations and palettes are registered in single constructors ([AnimationRegistry.cpp](src/animation/AnimationRegistry.cpp), [PaletteManager.cpp](src/PaletteManager.cpp)) — those are the registration choke points.
- Render work in `MqttLight::loop()` is guarded against redundant `applyHsv`/`show` calls via `prev*` caches; preserve that when modifying the render path to avoid pointless FastLED churn.
