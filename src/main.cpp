#include <WiFi.h>
#include <FastLED.h>
#include "SolidColorState.h"
#include "WebServerHandler.h"
#include "WifiCreds.h"

#define LED_PIN 23
#define NUM_LEDS 108
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];



LEDController* ledController;
WebServerHandler* webServerHandler;

void connectToWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to WiFi...");

  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < 30) {
    delay(1000);
    Serial.println(".");
    retryCount++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi!");
  }
}

void setup() {
  Serial.begin(115200);

  connectToWifi();

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  ledController = new LEDController(leds, NUM_LEDS);
  ledController->setState(new SolidColorState(CRGB::Black));

  webServerHandler = new WebServerHandler(ledController);
  webServerHandler->begin();
  
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 9000); 

  FastLED.clear();
  FastLED.show();

}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, attempting to reconnect...");
    connectToWifi();
  }
  
  ledController->update();
}
