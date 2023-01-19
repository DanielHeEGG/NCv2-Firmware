# NCv2-Firmware
Firmware for [Nixie Controller v2](https://github.com/DanielHeEGG/Nixie-Controller-v2).

## Features
- Driving arbitrary nixie tubes with adjustable tube current
- Internet time sync
- Web-based configuration interface

## Configuration
1. Power on the controller
2. Connect to WiFi named `NixieController-XXXXXXXXXXXX` where `XXXXXXXXXXXX` is a random hexadecimal number
3. Navigate to `192.168.0.2` in a browser
4. Enter SSID, password, tube current, and time zone information
5. Refresh the page to check if WiFi connection was successful

## Uploading Firmware
> The controller contains two microcontrollers (STM32 & ESP32) that both need to be flashed with their correct firmware
1. Open `MCU/src/main.c` and ***uncomment*** line 37 `// #define FLASH_MODE`
2. Compile STM32 firmware with [PlatformIO](https://platformio.org) according to `MCU/platformio.ini`
3. Power ***off*** the controller
4. Upload compiled binary via the JTAG pads on the controller PCB with an ARM debugger tool
5. Compile ESP32 firmware with [PlatformIO](https://platformio.org) according to `Comms/platformio.ini`
6. Power ***on*** the controller
7. Upload complied binary via the UART pads on the controller PCB with a USB to UART tool
8. Open `MCU/src/main.c` and ***comment*** line 37 `#define FLASH_MODE`
9. Compile ESP32 firmware with [PlatformIO](https://platformio.org) according to `MCU/platformio.ini`
10. Power ***off*** the controller
11. Upload compiled binary via the JTAG pads on the controller PCB with an ARM debugger tool
