#include "config/Config.h"

#define LIGHT_CONFIG_TOPIC "homeassistant/light/" MQTT_CLIENT_ID

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