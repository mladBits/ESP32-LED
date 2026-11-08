#pragma once

#if defined(DEVICE_ID_ESP32_PEG)
#define MQTT_CLIENT_ID  "esp32_peg_led"
#define MQTT_UNIQUE_ID  "esp32_led_5"
#define MQTT_NAME       "ESP32 Peg LED"

#elif defined(DEVICE_ID_ESP32_DESK)
#define MQTT_CLIENT_ID  "esp32_desk_led"
#define MQTT_UNIQUE_ID  "esp32_led_6"
#define MQTT_NAME       "ESP32 Desk LED"

#elif defined(DEVICE_ID_ESP32_WALL)
#define MQTT_CLIENT_ID  "esp32_wall_led"
#define MQTT_UNIQUE_ID  "esp32_led_7"
#define MQTT_NAME       "ESP32 Wall LED"

#endif

// `git describe` of the build, injected by scripts/fw_version.py.
#ifndef FW_VERSION
#define FW_VERSION "unknown"
#endif

// Where signed releases are published (scripts/release.py) and fetched from.
#define RELEASES_URL "https://github.com/mladBits/ESP32-LED/releases"