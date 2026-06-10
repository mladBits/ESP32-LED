# Unit Tests

Host-native tests (PlatformIO + Unity) for the pure-logic parts of the firmware.
No hardware needed — they compile and run on the development machine:

```bash
pio test -e native
```

| Suite                       | Covers                                                              |
|-----------------------------|---------------------------------------------------------------------|
| `test_home_assistant_color` | HA degrees/percent ↔ FastLED 0–255 conversions, clamping, round trips (`src/mqtt/HomeAssistantColor.h`) |
| `test_color_math`           | Shortest-path hue interpolation around the 0–255 wrap (`src/led/ColorMath.h`) |
| `test_animation_direction`  | Direction string parsing and defaulting (`src/animation/AnimationDirection.h`) |

Requires a host GCC on `PATH` (e.g. WinLibs MinGW on Windows: `winget install BrechtSanders.WinLibs.POSIX.UCRT`).

Hardware-facing code (FastLED rendering, animations, MQTT/WiFi) is intentionally
not unit-tested — it's verified on-device via Home Assistant.
