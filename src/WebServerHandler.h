#ifndef WEB_SERVER_HANDLER_H
#define WEB_SERVER_HANDLER_H

#include <ESPAsyncWebServer.h>
#include "LEDController.h"
#include "SolidColorState.h"
#include "PlasmaState.h"

class WebServerHandler {
    private:
        AsyncWebServer server;
        LEDController* ledController;

    public:
        WebServerHandler(LEDController* controller, int port = 80);
        void begin();
};

#endif