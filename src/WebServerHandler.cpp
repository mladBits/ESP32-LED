#include "WebServerHandler.h"
#include "LEDStateFactory.h"

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

            isOn = true;
            request->send(200, "application/json", "{\"success\": true, \"message\": \"color set!\"}");
        } else {
            request->send(400, "application/json", "{\"success\": false, \"message\": \"missing color parameters!\"}");
        }
    });

    server.on("/set-animation", HTTP_POST, [this](AsyncWebServerRequest* request) {
        LEDState* currentState = ledController->getState();
        if (request->hasParam("static", false)) {
            bool isStatic = request->getParam("static", false)->value().equalsIgnoreCase("true");
            currentState->setStatic(isStatic);
        }

        if (request->hasParam("paletteId", false)) {
            int paletteId = request->getParam("paletteId", false)->value().toInt();
            currentState->setPaletteId(paletteId);
        }

        LEDState* newState = nullptr;
        if (request->hasParam("style", false)) {
            int styleId = request->getParam("style", false)->value().toInt();
            newState = LEDStateFactory::createState(styleId, currentState, pm);
        }

        if (newState) {
            ledController->setState(newState);
            //delete newState;
        }

        isOn = true;
        request->send(200, "application/json", "{\"success\": true, \"message\": \"animation set!\"}");
    });

    server.on("/on", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!isOn) {
            fadeIn(2000);

            isOn = true;
            request->send(200, "text/plain", "LEDs on");
        }
    });

    server.on("/off", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (isOn) {
            fadeOut(2000);

            isOn = false;
            request->send(200, "text/plain", "LEDs off");
        }
    });

    server.on("/palettes", HTTP_GET, [this](AsyncWebServerRequest* request) {
        PaletteManager pm;
        request->send(200, "application/json", pm.getPaletteNamesInJson());
    });

    server.begin();
}