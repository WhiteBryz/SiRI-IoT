# SiRI IoT — Sistema de Riego Inteligente

Sistema de riego automatizado y monitoreo ambiental basado en ESP32. Combina lectura
de sensores, control de una electroválvula/bomba por relé, registro local en tarjeta
SD y reporte y control remoto por MQTT en formato JSON.

## Componentes del proyecto

Este repositorio contiene el firmware del dispositivo. El proyecto completo se
compone de tres repositorios:

- **Firmware** (este repositorio): lee los sensores, controla el riego y publica/recibe
  datos por MQTT.
- **[SiRI-API](https://github.com/WhiteBryz/SiRI-API)**: backend que conecta el
  dispositivo con los clientes finales.
- **[AimsMovilApp](https://github.com/JoelGarciaDev/AimsMovilApp)**: aplicación móvil
  para monitorear y controlar el sistema.

## Funcionalidad

- **Sensores**: temperatura y humedad ambiente (DHT11), intensidad de luz (LDR),
  nivel de agua del tanque (ultrasónico HC-SR04) y humedad de suelo (según la
  variante de firmware).
- **Modos de riego**:
  - **Manual**: un botón físico enciende/apaga la bomba.
  - **Automático**: la bomba se activa según umbrales configurables de luz y
    humedad.
  - **Por horario** (`SiRIM/`): la bomba se activa a una hora programada del día.
- **Seguridad**: si el nivel de agua del tanque cae por debajo del 20%, el riego se
  bloquea sin importar el modo activo.
- **Telemetría**: cada pocos segundos se publica un JSON por MQTT con fecha/hora,
  lecturas de sensores y estado de riego, y se guarda una copia en la tarjeta SD.
- **Control remoto**: el sistema se suscribe a un topic MQTT para recibir comandos en
  JSON que cambian el modo de riego, los umbrales o la hora programada en caliente.
- **Pantalla**: LCD I2C con el estado del sistema. En `CodigoIoTV1.0BETA/` incluye
  además un carrusel navegable de 5 pantallas con un botón físico.
- **Reloj de tiempo real**: módulo RTC DS1307 para fecha/hora, usado en la pantalla
  y en el modo de riego por horario.

## Implementaciones

El repositorio incluye dos variantes de firmware con la misma funcionalidad descrita
arriba, pensadas para el mismo hardware:

- **`CodigoIoTV1.0BETA/`** — arquitectura de un solo núcleo: todo el ciclo de
  lectura de sensores, control de riego, pantalla y comunicación MQTT corre en un
  único `loop()`.
- **`SiRIM/`** — arquitectura dual-core sobre FreeRTOS: la conexión WiFi/MQTT corre
  en una tarea dedicada a un núcleo del ESP32, y la lectura de sensores junto con el
  control de riego corre en una tarea separada en el otro núcleo.

## Pines

### `CodigoIoTV1.0BETA/`

| Función | Pin |
|---|---|
| Trigger ultrasónico | 26 |
| Echo ultrasónico | 25 |
| LDR (luz) | 35 |
| Relé de riego | 27 |
| DHT11 | 16 |
| SD — CS | 5 |
| SD/SPI — MOSI / MISO / SCK | 23 / 19 / 18 |
| Botón bomba manual | 0 |
| Botón carrusel de pantallas | 2 |

### `SiRIM/`

| Función | Pin |
|---|---|
| Trigger ultrasónico | 26 |
| Echo ultrasónico | 25 |
| LDR (luz) | 35 |
| Humedad de suelo 1 / 2 | 34 / 33 |
| Relé de riego 1 / 2 | 4 / 17 |
| DHT11 | 16 |
| SD — CS | 5 |
| SD/SPI — MOSI / MISO / SCK | 23 / 19 / 18 |
| Botón bomba manual | 15 |

## Contrato MQTT

Ambas variantes publican telemetría en el topic `ucol/iot/sensores` y reciben
comandos de configuración en el topic `ucol/iot/config`, ambos en JSON.

### `CodigoIoTV1.0BETA/`

Telemetría publicada:

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

Comando recibido:

```json
{ "modo": "manual", "nivelLuz": 100, "humedadSuelo": 100 }
```

`modo` puede ser `"manual"` o `"auto"`. `nivelLuz`/`humedadSuelo` son los umbrales
usados en modo automático.

### `SiRIM/`

Telemetría publicada:

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

Comando recibido:

```json
{
  "modo": "manual",
  "bombaOn": true,
  "nivelLuz": 100,
  "humedadSuelo": 100,
  "horaRiego": "09:05"
}
```

`modo` puede ser `"manual"`, `"auto"` o `"timer"`. Los demás campos son opcionales y
solo aplican según el modo: `bombaOn` en manual, `nivelLuz`/`humedadSuelo` en
automático, `horaRiego` (formato `HH:MM`) en riego por horario.

## Configuración

### Librerías Arduino requeridas

`Wire`, `SPI`, `SD`, `LiquidCrystal_I2C`, `DHT`, `RTClib`, `ArduinoJson`, `WiFi`,
`PubSubClient`.

### Credenciales (`env.h`)

Cada variante espera un archivo `env.h` (no versionado, agregado a `.gitignore`) en
su propia carpeta con la siguiente estructura mínima:

```cpp
struct KeysEnv {
  char* ssid = "[ssid]";
  char* password = "[password]";
  char* mqtt_server = "[servidor-mqtt]";
  uint16_t MQTT_PORT = 1883;          // solo necesario en SiRIM
  char* topicRX = "ucol/iot/config";  // solo necesario en SiRIM
  char* topicTX = "ucol/iot/sensores";// solo necesario en SiRIM
} KeysEnv;
```

## Estructura del repositorio

```
CodigoIoTV1.0BETA/   Firmware single-core
SiRIM/               Firmware dual-core (FreeRTOS)
examples/            Pruebas puntuales de sensores y conectividad
```
