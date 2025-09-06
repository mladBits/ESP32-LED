#include "config/Config.h"

#if defined(DEVICE_ID_ESP32_PEG)
#define LIGHT_CONFIG_TOPIC "homeassistant/light/esp32_peg_led"

#elif defined(DEVICE_ID_ESP32_DESK)
#define LIGHT_CONFIG_TOPIC "homeassistant/light/esp32_desk_led"

#elif defined(DEVICE_ID_ESP32_TEST)
#define LIGHT_CONFIG_TOPIC "homeassistant/light/" MQTT_CLIENT_ID
#endif

#define STATE_TOPIC        LIGHT_CONFIG_TOPIC "/state"
#define SET_TOPIC          LIGHT_CONFIG_TOPIC "/set"
#define CONFIG_TOPIC       LIGHT_CONFIG_TOPIC "/config"
#define PALETTE_SET_TOPIC  LIGHT_CONFIG_TOPIC "/palette/set1"
#define PALETTE_LIST_TOPIC LIGHT_CONFIG_TOPIC "/palette/list"