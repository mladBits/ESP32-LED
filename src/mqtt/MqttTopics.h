#pragma once

#include "config/Config.h"

#define LIGHT_CONFIG_TOPIC "homeassistant/light/" MQTT_CLIENT_ID

// Firmware updates: the HA `update` entity's state (retained JSON) and the
// command topic it sends "install" to ("check" re-checks for a release now).
#define UPDATE_STATE_TOPIC   LIGHT_CONFIG_TOPIC "/update/state"
#define UPDATE_COMMAND_TOPIC LIGHT_CONFIG_TOPIC "/update/command"
#define UPDATE_CONFIG_TOPIC  "homeassistant/update/" MQTT_UNIQUE_ID "_firmware/config"

#define STATE_TOPIC        LIGHT_CONFIG_TOPIC "/state"
#define SET_TOPIC          LIGHT_CONFIG_TOPIC "/set"
// Periodic device telemetry (power estimate, WiFi, heap, uptime) for metrics.
#define TELEMETRY_TOPIC    LIGHT_CONFIG_TOPIC "/telemetry"
#define CONFIG_TOPIC       LIGHT_CONFIG_TOPIC "/config"
// Retained "online"/"offline"; the broker publishes "offline" (our last will)
// when the connection drops without a clean disconnect.
#define AVAILABILITY_TOPIC LIGHT_CONFIG_TOPIC "/availability"
#define PALETTE_SET_TOPIC   LIGHT_CONFIG_TOPIC "/palette/set1"
#define PALETTE_LIST_TOPIC  LIGHT_CONFIG_TOPIC "/palette/list"
#define PALETTE_STATE_TOPIC LIGHT_CONFIG_TOPIC "/palette/state"
// Raw palette data (16 hex colors) applied directly, never persisted.
#define PALETTE_DATA_TOPIC  LIGHT_CONFIG_TOPIC "/palette/data"

// Home Assistant MQTT discovery for the palette as a `select` entity.
#define PALETTE_SELECT_CONFIG_TOPIC "homeassistant/select/" MQTT_UNIQUE_ID "_palette/config"