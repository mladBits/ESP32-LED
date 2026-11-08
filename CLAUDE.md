# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

Firmware for ESP32 boards that drive WS2812B/WS2811 addressable LED strips. The control path is **MQTT with Home Assistant discovery** — the device auto-registers as an HA `light` entity (plus a `select` entity for palettes) and is controlled via JSON command messages. Built with PlatformIO + Arduino framework + FastLED.

## Build / Upload / Monitor

PlatformIO is the toolchain. There are **multiple environments**, one per physical device, each selected with `-e`. There is no single "default" — always specify the environment.

```bash
pio run -e desk_esp32                    # compile
pio run -e desk_esp32 -t upload          # compile + flash over USB
pio run -e desk_esp32 -t upload -t monitor   # flash then open serial monitor
pio device monitor -b 115200             # serial monitor only (115200 baud)
pio run -t clean -e desk_esp32           # clean build artifacts
```

Environments (defined in [platformio.ini](platformio.ini)): `peg_esp32` (3 strips), `desk_esp32`, `wall_esp32`. Each also has an `*_ota` variant (e.g. `desk_esp32_ota`) that uploads over WiFi via ArduinoOTA/espota to the board's fixed LAN IP (`upload_port`; mDNS `.local` is unreliable on Windows); auth is injected from `OTA_PASSWORD` by [scripts/ota_auth.py](scripts/ota_auth.py). First flash must be USB. [scripts/fw_version.py](scripts/fw_version.py) (`[common]` `extra_scripts`) stamps `FW_VERSION` from `git describe --dirty`, published as the HA device's `sw_version`; an env that sets its own `extra_scripts` must keep `${common.extra_scripts}`. Each sets a `-DDEVICE_ID_ESP32_*` macro plus `LED_PIN_*` / `NUM_LEDS_*` / `LED_VOLTS` (LED supply voltage) build flags. Per-device identity (MQTT client/unique IDs, names) is resolved at compile time from that macro in [src/config/Config.h](src/config/Config.h); topic prefixes in [src/mqtt/MqttTopics.h](src/mqtt/MqttTopics.h) derive from `MQTT_CLIENT_ID`. **Adding a board = add an env block in `platformio.ini` and a matching `#elif` branch in `Config.h`** (and the strip-allocation block in [src/main.cpp](src/main.cpp)).

Unit tests run on the host (no hardware): `pio test -e native`. They cover only the pure-logic headers — [src/mqtt/HomeAssistantColor.h](src/mqtt/HomeAssistantColor.h), [src/led/ColorMath.h](src/led/ColorMath.h), [src/animation/AnimationDirection.h](src/animation/AnimationDirection.h), [src/animation/AnimMath.h](src/animation/AnimMath.h), [src/net/Version.h](src/net/Version.h), [src/net/ReleaseFormat.h](src/net/ReleaseFormat.h) — which must stay Arduino-free to remain testable. The `native` env excludes all firmware sources (`build_src_filter = -<*>`); a host GCC must be on PATH.

CI ([.github/workflows/ci.yml](.github/workflows/ci.yml)) runs on every push: the native tests, plus a build of each board env on Linux (so filename case matters) **without** the credential headers, i.e. a release-style, secret-free build. **Adding a board means adding it to the workflow's matrix too.**

Releases come from [scripts/release.py](scripts/release.py), run on the PC with PlatformIO's Python (+ `cryptography`), never in CI: the ECDSA P-256 private key stays at `~/.esp32-led/release-signing-key.pem` (backup in Proton Drive `backup\esp32-led\`), its public half is [src/config/ReleaseKey.h](src/config/ReleaseKey.h). It builds every board from a clean header-free checkout at an annotated `vX.Y.Z` tag, signs `board/version/size/sha256` per image (the exact message format is in the script's docstring; firmware verification must reproduce it byte for byte), and publishes the images plus `manifest.json` as a GitHub Release. Its `BOARDS` map (env → `MQTT_CLIENT_ID`) needs a new board too. Always `--dry-run` first.

Boards install releases themselves: [src/net/FirmwareUpdater.cpp](src/net/FirmwareUpdater.cpp) fetches the latest `manifest.json` over HTTPS (roots in [src/net/RootCAs.h](src/net/RootCAs.h), regenerate with `scripts/gen_root_cas.py` if GitHub changes CA), verifies the board's entry with mbedTLS against `ReleaseKey.h` using [ReleaseFormat.h](src/net/ReleaseFormat.h), and streams the image into the spare OTA slot, activating it only if size and SHA-256 match the signed values; [src/mqtt/UpdateEntity.cpp](src/mqtt/UpdateEntity.cpp) is the HA `update` entity around it (offers only `isNewerRelease()` per [Version.h](src/net/Version.h); installs run from `loop()`, never inside the MQTT callback). **Every new image boots on probation**: `verifyRollbackLater()` (must stay `extern "C"`, the core's weak default is C) stops the core auto-confirming it; reaching MQTT calls `FirmwareUpdater::markHealthy()`; otherwise after 10 min (or 3 boots) it rolls back — via the bootloader where it supports rollback (these boards' does: telemetry `ota_state` "pending"), else via the NVS slot record that `armProbation()` writes (called after every install and from ArduinoOTA `onEnd`). Keep `FirmwareUpdater::begin()` first in `setup()`. A test build with `PLATFORMIO_BUILD_FLAGS="-DFW_TEST_NEVER_HEALTHY -DFW_HEALTH_TIMEOUT_MS=90000UL"` exercises the rollback.

## Credentials (local headers → board NVS)

WiFi, MQTT and OTA settings live in each board's NVS storage ([src/config/Credentials.cpp](src/config/Credentials.cpp), `Preferences` namespace `creds`), which survives reboots and firmware updates. Two `.gitignore`d headers are their master copy — never commit them:

- `src/config/WifiCreds.h` — defines `WIFI_SSID`, `WIFI_PASSWORD`, `OTA_PASSWORD`
- `src/config/Mqtt.h` — defines `MQTT_SERVER`, `MQTT_PORT`, `MQTT_USER`, `MQTT_PASSWORD`

A build **with** them (any local build) uses their values and saves them to NVS when NVS differs, so one normal OTA upload provisions or updates a board. A build **without** them (CI, releases) compiles fine and uses NVS only, so the firmware file contains no secrets. Code reads the global `credentials` struct, never the macros (only `Credentials.cpp` includes the headers). Telemetry `creds_stored` shows whether a board's NVS holds a complete set (i.e. whether a header-free build would boot there). `scripts/ota_auth.py` still reads `OTA_PASSWORD` from the local header to authenticate uploads. Backups of the headers: Proton Drive `backup\esp32-led\config\`. To give a board away, erase its whole flash (`pio run -t erase`) — reflashing alone leaves NVS intact.

## Architecture

The control flow is a one-directional pipeline: **MQTT message → `MqttLight` (intent/state) → `LEDController` (per-strip ops) → `Strip` (state) → `Animation` (renders into the strip's `CRGB[]`) → `FastLED.show()`**.

- **[src/main.cpp](src/main.cpp)** — allocates the `CRGB[]` arrays statically (count/pins from build flags), registers strips with FastLED and the `LEDController`, wires up MQTT, and runs the loop. Chipset/color order is chosen per device (`WALL` = WS2811/RGB, others = WS2812B/GRB).

- **[src/mqtt/MqttLight.cpp](src/mqtt/MqttLight.cpp)** — the brain. Owns all user-facing state (power, brightness, hue/sat, selected effect, blend state). `loop()` runs every tick: handles MQTT reconnect, smoothly ramps brightness toward `targetBrightness`, and either drives the selected animation or renders a blended solid color. Key design points:
  - **Brightness is applied via `FastLED.setBrightness`, never as the HSV `V` channel** — solid colors are always rendered at full V.
  - Solid color changes **interpolate** hue (shortest path around the 255-wrap circle) and saturation over `solidBlendDurationMs` (~1s).
  - `callback()` parses HA JSON commands on `SET_TOPIC` (state/brightness/color/effect/`effect_dir`) and palette selections on `PALETTE_SET_TOPIC`.
  - On (re)connect it publishes HA discovery config (`publishDeviceConfig`) and the palette list, then subscribes.

- **[src/led/LEDController.cpp](src/led/LEDController.cpp)** — thin fan-out layer. Holds up to `MAX_STRIPS` (8) `Strip`s and applies every operation (clear, fill solid, animate, set palette/direction) across all of them. Stateless beyond the strip array + `isOn`.

- **[src/led/Strip.h](src/led/Strip.h)** — per-strip state struct: `leds` pointer + count, `currentPalette`/`targetPalette` (animations blend current toward target), `direction`, and the active `Animation*`.

- **Animations** ([src/animation/](src/animation/)) — each implements the `Animation` interface (`update(Strip&)` + `getName()`). They are **singletons**: instantiated once in [AnimationRegistry.cpp](src/animation/AnimationRegistry.cpp), looked up by name, and write directly into `strip.leds`. One instance renders every strip, so per-strip variation must be keyed by `strip.id`. **Animations are pure functions of the shared clock** so boards stay in sync: [AnimClock](src/animation/AnimClock.h) is SNTP-synced epoch ms, latched once per frame by `LEDController::animate()`. Derive motion from `AnimClock::now()`, never from `millis()` or from accumulators integrated frame by frame — use `beatsinIntegral16` ([AnimMath.h](src/animation/AnimMath.h)) for the closed form of an `acc += deltams * beatsin88(...)` speed — and pass `AnimClock::timebase()` to every FastLED `beat*()`/`beatsin*()` call (they read local `millis()` otherwise). Palette-blend rate limiting (`paletteLastBlendMs`) may stay on `millis()`. Compare timestamps as elapsed time (`now - start >= dur`), never absolute (`now >= start + dur`), so clock wraparound is safe. `NTP_SERVER` (build flag, default `pool.ntp.org`) picks the time server; telemetry's `clock_synced` shows whether a board has synced. The registry's `list()`/`getByName()` also feed the HA `effect_list`. **To add an animation: create the class, add the member + `animations[count++]` line in the registry constructor** (and bump `capacity` in [AnimationRegistry.h](src/animation/AnimationRegistry.h) if full).

- **[src/led/PaletteManager.cpp](src/led/PaletteManager.cpp)** — name→`CRGBPalette16` map; serializes the available palette names to JSON for the `palette/list` topic and resolves incoming palette selections.

- **[src/mqtt/HomeAssistantColor.h](src/mqtt/HomeAssistantColor.h)** — conversions between HA's hue-degrees/sat-percent and FastLED's 0–255 hue/sat. Anywhere color crosses the MQTT boundary, it goes through these.

- **[src/net/NetworkManager.h](src/net/NetworkManager.h)** — static WiFi helpers: blocking `connect()` for setup() only; non-blocking, rate-limited `ensureConnected()` called every loop() tick so rendering survives WiFi drops.

## Conventions

- Device-specific behavior is handled with **compile-time `#if defined(DEVICE_ID_ESP32_*)` branches**, not runtime config. When touching anything device-specific, update every branch.
- Animations and palettes are registered in single constructors ([AnimationRegistry.cpp](src/animation/AnimationRegistry.cpp), [PaletteManager.cpp](src/led/PaletteManager.cpp)) — those are the registration choke points.
- Render work in `MqttLight::loop()` is guarded against redundant `applyHsv`/`show` calls via `prev*` caches; preserve that when modifying the render path to avoid pointless FastLED churn.
