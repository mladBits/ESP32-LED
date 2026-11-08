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
// Palettes come from palette-service (homelab repo), which owns the palette
// library and this board's Palette dropdown in HA; the board keeps none.
// Current palette name, retained: {"palette": "<name>"}.
#define PALETTE_STATE_TOPIC LIGHT_CONFIG_TOPIC "/palette/state"
// The palette to show: {"name": "<name>", "colors": ["#RRGGBB" x16]}, sent
// by palette-service when one is picked (no name shows as "Custom").
#define PALETTE_DATA_TOPIC  LIGHT_CONFIG_TOPIC "/palette/data"
// Retained announcement that palette-service discovers boards by:
// {"unique_id", "name", "palette"}. Republished on every connect, which also
// makes the service resend the current palette's colours.
#define PALETTE_BOARD_TOPIC LIGHT_CONFIG_TOPIC "/palette/board"