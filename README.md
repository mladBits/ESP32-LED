# ESP32 LED Strip Controller

This project provides a C/C++ implementation of an HTTP REST service that runs on an ESP32 microcontroller to control/manage a set of LED strips. With this service, users can configure LED strips to display solid colors or animations.

## Features
- **REST API**: Control LED strips via HTTP POST requests.
- **Solid Color Mode**: Set LED strips to a single colour of your choice.
- **Animation Mode**: Set LED strips to an animation. Currently includes one animation with plans to add more.

## Hardware Requirements
- **ESP32**: NodeMCU32 or any other ESP32-based board.
- **LED Strips**: Something that is compatible with ESP32 GPIO (ie WS2812, WS2811, or other addressable LEDs).
- **Power Supply**: Something with the correct voltage and sufficient current that can power the total number of LEDs.

## Software Requirements
- **PlatformIO**: Recommended development environment.
- **Arduino Core for ESP32**: Required for compiling and uploading code.

## Software Setup
1. Clone this repository:
  `git clone https://github.com/mladBits/ESP32-LED.git`
2. Open project in PlatformIO.
3. Configure the project settings if needed (ie WIFI, GPIO pin).
4. Build and upload code.

## REST API Endpoints
todo

## Future Enhancements
todo

