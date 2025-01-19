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
        if (request->hasParam("static", false)) {
            bool isStatic = request->getParam("static", false)->value().equalsIgnoreCase("true");
            ledController->getState()->setStatic(isStatic);
        }

        if (request->hasParam("paletteId", false)) {
            int paletteId = request->getParam("paletteId", false)->value().toInt();
            ledController->getState()->setPaletteId(paletteId);
        }
        ledController->setState(
            new PlasmaState(
                pm->getPaletteById(ledController->getState()->getPaletteId()), 
                ledController->getState()->getStatic()));

        request->send(200, "application/json", "{\"success\": true, \"message\": \"animation set!\"}");
    });

    server.on("/power", HTTP_POST, [this](AsyncWebServerRequest* request) {
        String action = "";

        if (request->hasParam("action", false)) {
            action = request->getParam("action", false)->value();
        }

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