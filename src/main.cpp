#include <WiFi.h>
#include <ArduinoOTA.h>
#include <FastLED.h>
#include "mqtt/MqttLight.h"
#include "led/LEDController.h"
#include "net/NetworkManager.h"
#include "animation/AnimClock.h"
#include "config/Config.h"
#include "config/Credentials.h"
#include "net/FirmwareUpdater.h"

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
// The pointers are fixed now; loadCredentials() fills them in setup().
MqttLight mqttLight(mqttClient, credentials.mqttUser, credentials.mqttPassword);

void setup() {
  Serial.begin(115200);
  // First, so a new image that crashes during setup is still counted (and
  // rolled back after repeated reboots). See FirmwareUpdater.h.
  FirmwareUpdater::begin();
  delay(2000);

  loadCredentials();
  NetworkManager::connect();
  // Shared clock so animations line up across boards (SNTP retries if
  // WiFi isn't up yet).
  AnimClock::begin();

  ArduinoOTA.setHostname(MQTT_CLIENT_ID);
  ArduinoOTA.setPassword(credentials.otaPassword);
  ArduinoOTA.onStart([]() {
    // blank the strips: RMT interrupts during a show() could disturb the transfer
    FastLED.clear();
    FastLED.show();
    Serial.println("OTA update starting...");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA progress: %u%%\r", (progress * 100) / total);
  });
  // A PC upload gets the same probation as a release install.
  ArduinoOTA.onEnd([]() { FirmwareUpdater::armProbation(); });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error[%u]\n", error);
  });
  // Never listen for uploads without a password (an unprovisioned board).
  if (credentials.otaPassword[0]) {
    ArduinoOTA.begin();
  } else {
    Serial.println("No OTA password stored; OTA uploads disabled");
  }

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

#if defined(DEVICE_ID_ESP32_PEG)
  // Powered by a Mean Well GST60A05-P1J (5V/6A). Cap below its rating with
  // derating headroom (~4.75A total incl. the ESP32) so a full-white scene
  // can't overload it. Real scenes peak ~3.5A, well under this.
  FastLED.setMaxPowerInVoltsAndMilliamps(LED_VOLTS, 4500);
#elif defined(DEVICE_ID_ESP32_WALL)
  FastLED.setMaxPowerInVoltsAndMilliamps(LED_VOLTS, 4000);
#else
  FastLED.setMaxPowerInVoltsAndMilliamps(LED_VOLTS, 9000);
#endif

  FastLED.setBrightness(75);
  
  mqttClient.setBufferSize(2048);
  // MQTT connect() blocks rendering twice: the TCP connect (WiFiClient,
  // default 3s, e.g. broker host powered off), then the CONNACK wait
  // (PubSubClient, default 15s, broker up but unresponsive). Keep both short.
  espClient.setTimeout(1);         // seconds
  mqttClient.setSocketTimeout(2);  // seconds
  mqttClient.setServer(credentials.mqttServer, credentials.mqttPort);

  mqttLight.setController(&ledController);
  mqttLight.begin();
  
  FastLED.clear();
  FastLED.show();
}

void loop() {
  ArduinoOTA.handle();
  FirmwareUpdater::loop();
  NetworkManager::ensureConnected();
  mqttLight.loop();
}
