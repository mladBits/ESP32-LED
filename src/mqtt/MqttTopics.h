#include "config/Config.h"

#if defined(DEVICE_ID_ESP32_PEG)
#define LIGHT_CONFIG_TOPIC "homeassistant/light/esp32_peg_led"

#elif defined(DEVICE_ID_ESP32_DESK)
#define LIGHT_CONFIG_TOPIC "homeassistant/light/esp32_desk_led"

#elif defined(DEVICE_ID_ESP32_WALL)
#define LIGHT_CONFIG_TOPIC "homeassistant/light/esp32_wall_led"

#elif defined(DEVICE_ID_ESP32_TEST)
#define LIGHT_CONFIG_TOPIC "homeassistant/light/" MQTT_CLIENT_ID
#endif

#define STATE_TOPIC        LIGHT_CONFIG_TOPIC "/state"
#define SET_TOPIC          LIGHT_CONFIG_TOPIC "/set"
// Periodic device telemetry (power estimate, WiFi, heap, uptime) for metrics.
#define TELEMETRY_TOPIC    LIGHT_CONFIG_TOPIC "/telemetry"
#define CONFIG_TOPIC       LIGHT_CONFIG_TOPIC "/config"
#define PALETTE_SET_TOPIC   LIGHT_CONFIG_TOPIC "/palette/set1"
#define PALETTE_LIST_TOPIC  LIGHT_CONFIG_TOPIC "/palette/list"
#define PALETTE_STATE_TOPIC LIGHT_CONFIG_TOPIC "/palette/state"
// Raw palette data (16 hex colors) applied directly, never persisted.
#define PALETTE_DATA_TOPIC  LIGHT_CONFIG_TOPIC "/palette/data"

// Home Assistant MQTT discovery for the palette as a `select` entity.
#define PALETTE_SELECT_CONFIG_TOPIC "homeassistant/select/" MQTT_UNIQUE_ID "_palette/config"