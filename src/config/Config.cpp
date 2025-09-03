#include "Mqtt.h"

#if defined(DEVICE_ID_ESP32_PEG)
const char* MQTT_CLIENT_ID = "esp32_peg_led";
const char* MQTT_UNIQUE_ID = "esp32_led_5";
const char* MQTT_NAME = "ESP32 Peg LED";
const char* LIGHT_CONFIG_TOPIC = "homeassistant/light/esp32_peg_led";

#elif defined(DEVICE_ID_ESP32_DESK)
const char* MQTT_CLIENT_ID = "esp32_desk_led";
const char* MQTT_UNIQUE_ID = "esp32_led_6";
const char* MQTT_NAME = "ESP32 Desk LED";
const char* LIGHT_CONFIG_TOPIC = "homeassistant/light/esp32_desk_led";

#elif defined(DEVICE_ID_ESP32_TEST)
const char* MQTT_CLIENT_ID = "esp32_test_led_66";
const char* MQTT_UNIQUE_ID = "esp32_test_led_66";
const char* MQTT_NAME = "ESP32 Test LED 66";
const char* LIGHT_CONFIG_TOPIC = "homeassistant/light/esp32_test_led";

#endif