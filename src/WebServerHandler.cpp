#include "WebServerHandler.h"

WebServerHandler::WebServerHandler(LEDController* controller, int port)
    : server(port), ledController(controller) {}

void fadeIn(int fadeTime) {
    for (int i = 0; i <= 255; i++) {
        FastLED.setBrightness(i);
        FastLED.show();
        delay(fadeTime / 255);
    }
}

void fadeOut(int fadeTime) {
    for (int i = 255; i >= 0; i--) {
        FastLED.setBrightness(i);
        FastLED.show();
        delay(fadeTime / 255);
    }
}

void WebServerHandler::begin() {
    server.on("/set-color", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (request->hasParam("r", false) && request->hasParam("g", false) && request->hasParam("b", false)) {
            int r = request->getParam("r", false)->value().toInt();
            int g = request->getParam("g", false)->value().toInt();
            int b = request->getParam("b", false)->value().toInt();
            ledController->setState(new SolidColorState(CRGB(r, g, b)));
            request->send(200, "application/json", "{\"success\": true, \"message\": \"color set!\"}");
        } else {
            request->send(400, "application/json", "{\"success\": false, \"message\": \"missing color parameters!\"}");
        }
    });

    server.on("/set-animation", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (request->hasParam("type", false)) {
            String type = request->getParam("type", false)->value();

            if (type == "1") {
                ledController->setState(new PlasmaState(OceanColors_p));
                request->send(200, "application/json", "{\"success\": true, \"message\": \"plasma animation set!\"}");
            } else {
                request->send(400, "application/json", "{\"success\": false, \"message\": \"invalid animation type!\"}");
            }
        }
    });

    server.on("/power", HTTP_POST, [this](AsyncWebServerRequest* request) {
        String action = "";

        if (request->hasParam("action", false)) {
            action = request->getParam("action", false)->value();
        }

        Serial.println("Recieved action: " + action);

        if (action == "on") {
            fadeIn(2000);
            request->send(200, "text/plain", "LEDs on");
        } else if (action == "off") {
            fadeOut(2000);
            request->send(200, "text/plain", "LEDs off");
        } else {
            request->send(400, "text/plain", "invalid action");
        }
    });

    server.begin();
}