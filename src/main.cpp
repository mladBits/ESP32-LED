#include <WiFi.h>
#include <FastLED.h>
#include "mqtt/MqttLight.h"
#include "LEDController.h"
#include "NetworkManager.h"
#include "config/Mqtt.h"

#if defined(DEVICE_ID_ESP32_WALL)
  #define LED_CHIPSET WS2811
  #define LED_COLOR_ORDER RGB
#else
  #define LED_CHIPSET WS2812B
  #define LED_COLOR_ORDER GRB
#endif

#if defined(DEVICE_ID_ESP32_PEG)
CRGB leds0[NUM_LEDS_0];
CRGB leds1[NUM_LEDS_1];
CRGB leds2[NUM_LEDS_2];
#elif defined(DEVICE_ID_ESP32_DESK)
CRGB leds0[NUM_LEDS_0];
#elif defined(DEVICE_ID_ESP32_WALL)
CRGB leds0[NUM_LEDS_0];
#elif defined(DEVICE_ID_ESP32_TEST)
CRGB leds0[NUM_LEDS_0];
#else
#error "Unknown device environment"
#endif

template<uint8_t PIN>
static inline void addStrip(CRGB* leds, int count) {
  FastLED.addLeds<LED_CHIPSET, PIN, LED_COLOR_ORDER>(leds, count)
         .setCorrection(TypicalLEDStrip);
}

WiFiClient espClient;
PubSubClient mqttClient(espClient);
MqttLight mqttLight(mqttClient, MQTT_USER, MQTT_PASSWORD);

void setup() {
  Serial.begin(115200);
  delay(2000);

  NetworkManager::connect();
  static LEDController ledController;

#if defined(DEVICE_ID_ESP32_PEG)
    addStrip<LED_PIN_0>(leds0, NUM_LEDS_0);
    addStrip<LED_PIN_1>(leds1, NUM_LEDS_1);
    addStrip<LED_PIN_2>(leds2, NUM_LEDS_2);

    ledController.addStrip(leds0, NUM_LEDS_0);
    ledController.addStrip(leds1, NUM_LEDS_1);
    ledController.addStrip(leds2, NUM_LEDS_2);
#elif defined(DEVICE_ID_ESP32_DESK) || defined(DEVICE_ID_ESP32_WALL) || defined(DEVICE_ID_ESP32_TEST)
    addStrip<LED_PIN_0>(leds0, NUM_LEDS_0);
    ledController.addStrip(leds0, NUM_LEDS_0);
#endif

#if defined(DEVICE_ID_ESP32_WALL)
  FastLED.setMaxPowerInVoltsAndMilliamps(12, 4000);
#else
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 9000);
#endif

  FastLED.setBrightness(75);
  
  mqttClient.setBufferSize(2048);
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  mqttLight.setController(&ledController);
  mqttLight.begin();
  
  FastLED.clear();
  FastLED.show();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, attempting to reconnect...");
    NetworkManager::connect();
  }

  mqttLight.loop();
}
