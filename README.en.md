# SiRI IoT — Smart Irrigation System

*[Leer en español](README.md)*

Automated irrigation and environmental monitoring system based on ESP32. It combines
sensor readings, relay-driven control of a solenoid valve/pump, local logging to an
SD card, and remote reporting and control over MQTT in JSON.

## Project components

This repository holds the device firmware. The full project is made up of three
repositories:

- **Firmware** (this repository): reads the sensors, controls irrigation, and
  publishes/receives data over MQTT.
- **[SiRI-API](https://github.com/WhiteBryz/SiRI-API)**: backend that connects the
  device to the end clients.
- **[AimsMovilApp](https://github.com/JoelGarciaDev/AimsMovilApp)**: mobile app to
  monitor and control the system.

## Functionality

- **Sensors**: ambient temperature and humidity (DHT11), light intensity (LDR),
  tank water level (HC-SR04 ultrasonic), and soil moisture (depending on the
  firmware variant).
- **Irrigation modes**:
  - **Manual**: a physical button turns the pump on/off.
  - **Automatic**: the pump activates based on configurable light and humidity
    thresholds.
  - **Scheduled** (`SiRIM/`): the pump activates at a programmed time of day.
- **Safety**: if the tank's water level drops below 20%, irrigation is blocked
  regardless of the active mode.
- **Telemetry**: every few seconds a JSON payload with date/time, sensor readings,
  and irrigation status is published over MQTT and saved to the SD card.
- **Remote control**: the system subscribes to an MQTT topic to receive JSON
  commands that change the irrigation mode, thresholds, or scheduled time on the fly.
- **Display**: I2C LCD showing system status. `CodigoIoTV1.0BETA/` additionally
  includes a navigable 5-screen carousel driven by a physical button.
- **Real-time clock**: DS1307 RTC module for date/time, used by the display and by
  the scheduled irrigation mode.

## Implementations

The repository includes two firmware variants with the same functionality described
above, targeting the same hardware:

- **`CodigoIoTV1.0BETA/`** — single-core architecture: the whole cycle of sensor
  reading, irrigation control, display, and MQTT communication runs in a single
  `loop()`.
- **`SiRIM/`** — dual-core architecture on FreeRTOS: WiFi/MQTT connectivity runs in
  a task pinned to one ESP32 core, while sensor reading and irrigation control run
  in a separate task on the other core.

## Pinout

### `CodigoIoTV1.0BETA/`

| Function | Pin |
|---|---|
| Ultrasonic trigger | 26 |
| Ultrasonic echo | 25 |
| LDR (light) | 35 |
| Irrigation relay | 27 |
| DHT11 | 16 |
| SD — CS | 5 |
| SD/SPI — MOSI / MISO / SCK | 23 / 19 / 18 |
| Manual pump button | 0 |
| Screen carousel button | 2 |

### `SiRIM/`

| Function | Pin |
|---|---|
| Ultrasonic trigger | 26 |
| Ultrasonic echo | 25 |
| LDR (light) | 35 |
| Soil moisture 1 / 2 | 34 / 33 |
| Irrigation relay 1 / 2 | 4 / 17 |
| DHT11 | 16 |
| SD — CS | 5 |
| SD/SPI — MOSI / MISO / SCK | 23 / 19 / 18 |
| Manual pump button | 15 |

## MQTT contract

Both variants publish telemetry on the `ucol/iot/sensores` topic and receive
configuration commands on the `ucol/iot/config` topic, both in JSON.

### `CodigoIoTV1.0BETA/`

Published telemetry:

```json
{
  "fecha": "17/9/2026",
  "hora": "14:32:07",
  "timestamp": 1789000000,
  "temperaturaAmbiente": 24.5,
  "humedadAmbiente": 55.2,
  "humedadSuelo": 62,
  "iluminacion": 40,
  "nivelAgua": 78
}
```

Received command:

```json
{ "modo": "manual", "nivelLuz": 100, "humedadSuelo": 100 }
```

`modo` can be `"manual"` or `"auto"`. `nivelLuz`/`humedadSuelo` are the thresholds
used in automatic mode.

### `SiRIM/`

Published telemetry:

```json
{
  "fecha": "17/9/2026",
  "hora": "14:32:07",
  "temperaturaAmbiente": 24.5,
  "humedadAmbiente": 55.2,
  "humedadSuelo": { "sensor1": 60, "sensor2": 64 },
  "iluminacion": 40,
  "riegoManual": false,
  "nivelAgua": 78
}
```

Received command:

```json
{
  "modo": "manual",
  "bombaOn": true,
  "nivelLuz": 100,
  "humedadSuelo": 100,
  "horaRiego": "09:05"
}
```

`modo` can be `"manual"`, `"auto"`, or `"timer"`. The remaining fields are optional
and only apply depending on the mode: `bombaOn` in manual, `nivelLuz`/`humedadSuelo`
in automatic, `horaRiego` (format `HH:MM`) in scheduled irrigation.

## Setup

### Required Arduino libraries

`Wire`, `SPI`, `SD`, `LiquidCrystal_I2C`, `DHT`, `RTClib`, `ArduinoJson`, `WiFi`,
`PubSubClient`.

### Credentials (`env.h`)

Each variant expects an `env.h` file (not versioned, added to `.gitignore`) in its
own folder with the following minimal structure:

```cpp
struct KeysEnv {
  char* ssid = "[ssid]";
  char* password = "[password]";
  char* mqtt_server = "[mqtt-server]";
  uint16_t MQTT_PORT = 1883;          // only needed in SiRIM
  char* topicRX = "ucol/iot/config";  // only needed in SiRIM
  char* topicTX = "ucol/iot/sensores";// only needed in SiRIM
} KeysEnv;
```

## Repository structure

```
CodigoIoTV1.0BETA/   Single-core firmware
SiRIM/               Dual-core firmware (FreeRTOS)
examples/            Standalone sensor and connectivity tests
```
