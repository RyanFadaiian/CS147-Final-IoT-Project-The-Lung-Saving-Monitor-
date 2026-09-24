# The Lung-Saving Monitor

ESP32 air-quality monitor for a CS147 IoT final project. It reads a DHT11 temperature/humidity sensor and a PMS5003 particulate sensor, lights an alert LED when air quality is poor, and posts telemetry to Azure IoT Hub.

## Project Layout

- `platformio.ini` - PlatformIO configuration for the ESP32 dev board.
- `src/main.cpp` - Firmware entry point.
- `include/secrets.example.h` - Template for local WiFi and Azure credentials.
- `include/secrets.h` - Local credentials file, ignored by git.
- `lib/` - Space for project-specific PlatformIO libraries.
- `test/` - Space for PlatformIO tests.

## Setup

1. Install PlatformIO.
2. Copy `include/secrets.example.h` to `include/secrets.h`.
3. Fill in your WiFi credentials and Azure IoT Hub SAS token.
4. Build and upload:

```sh
pio run
pio run --target upload
pio device monitor
```

## Hardware Notes

- DHT11 data pin: GPIO5
- Red LED: GPIO2
- PMS5003 UART: TX to GPIO16, RX to GPIO17
