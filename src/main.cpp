#include <WiFi.h>
#include <FastLED.h>
#include "MqttLight.h"
#include "LEDController.h"
#include "NetworkManager.h"
#include "config/Mqtt.h"

#define ADD_LEDS(pin, count, array) FastLED.addLeds<WS2812B, pin, GRB>(array, count)

#if defined(DEVICE_ID_ESP32_PEG)
CRGB leds0[NUM_LEDS_0];
CRGB leds1[NUM_LEDS_1];
CRGB leds2[NUM_LEDS_2];
#elif defined(DEVICE_ID_ESP32_DESK)
CRGB leds0[NUM_LEDS_0];
#elif defined(DEVICE_ID_ESP32_TEST)
CRGB leds0[NUM_LEDS_0];
#else
#error "Unknown device environment"
#endif

WiFiClient espClient;
PubSubClient mqttClient(espClient);
MqttLight mqttLight(mqttClient, MQTT_USER, MQTT_PASSWORD);

void setup() {
  Serial.begin(115200);
  delay(2000);

  NetworkManager::connect();

  LEDController* ledController = new LEDController();

  #if defined(DEVICE_ID_ESP32_PEG)
    ADD_LEDS(LED_PIN_0, NUM_LEDS_0, leds0);
    ADD_LEDS(LED_PIN_1, NUM_LEDS_1, leds1);
    ADD_LEDS(LED_PIN_2, NUM_LEDS_2, leds2);

    ledController->addStrip(leds0, NUM_LEDS_0);
    ledController->addStrip(leds1, NUM_LEDS_1);
    ledController->addStrip(leds2, NUM_LEDS_2);
  #elif defined(DEVICE_ID_ESP32_DESK)
    ADD_LEDS(LED_PIN_0, NUM_LEDS_0, leds0);
    ledController->addStrip(leds0, NUM_LEDS_0);
  #elif defined(DEVICE_ID_ESP32_TEST)
    ADD_LEDS(LED_PIN_0, NUM_LEDS_0, leds0);
    ledController->addStrip(leds0, NUM_LEDS_0);
  #endif

  FastLED.setMaxPowerInVoltsAndMilliamps(5, 9000); 
  FastLED.setBrightness(150);
  
  mqttLight.setController(ledController);
  mqttClient.setBufferSize(1024);
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
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
