#pragma once

#if defined(DEVICE_ID_ESP32_PEG)
#define MQTT_CLIENT_ID  "esp32_peg_led"
#define MQTT_UNIQUE_ID  "esp32_led_5"
#define MQTT_NAME       "ESP32 Peg LED"

#elif defined(DEVICE_ID_ESP32_DESK)
#define MQTT_CLIENT_ID  "esp32_desk_led"
#define MQTT_UNIQUE_ID  "esp32_led_6"
#define MQTT_NAME       "ESP32 Desk LED"

#elif defined(DEVICE_ID_ESP32_TEST)
#define MQTT_CLIENT_ID  "esp32_test_led"
#define MQTT_UNIQUE_ID  "esp32_test_led_66"
#define MQTT_NAME       "ESP32 Test LED 66"

#endif