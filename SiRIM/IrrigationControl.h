#ifndef IrrigationControl_h
#define IrrigationControl_h

// Librerías
// #include "RTC.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <RTClib.h>
#include <ArduinoJson.h>

// Pines y configuración de dispositivos
#define TRIGGER 26
#define ECHO 25
#define VELOCIDAD_SONIDO 0.034
#define LDR_PIN 35
#define SOIL_MOISTURE1_PIN 34
#define SOIL_MOISTURE2_PIN 33
#define RELAY1_PIN 4
#define RELAY2_PIN 17
#define DHT_PIN 16
#define SD_CS 5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18
#define BTN_PIN1 15
#define BTN_PIN2 27 // reservado para el futuro botón de carrusel de LCD

// Instancias de las clases
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHT_PIN, DHT11);
RTC_DS1307 rtc;

struct ChangeConfiguration
{
  String setIrrigationTime;
  int setLightThreshold;
  int setSoilMoistureThreshold;
  bool setManualIrrigationMode;
  bool setTimerIrrigationMode;
  bool setIrrigationStatus;
};

class IrrigationControl
{
private:
  int counterForTimer = 0;
  // Lectura de los Sensores
  DateTime currentDate;
  int s_soilMoisture1;
  int s_soilMoisture2;
  int s_lightIntensity;
  float s_airHumidity;
  float s_airTemperature;
  float s_waterLevel;
  bool s_relayActive = false;
  unsigned long lastPumpButtonPress = 0;

  /*-- Parámetros de configuración para gestionar el riego (se modifica por medio de mensaje MQTT en JSON) --*/
  String irrigationAtTime = "";
  int secondsSetForWatering = 10;

  int minLightThreshold = 100;
  int minSoilMoistureThreshold = 100;

  // Condiciones para riego
  bool manualIrrigationActivated = false;
  bool timerIrrigationActivated = false;

public:
  // Estado de riego
  bool irrigationStatus = false;

  /*--- Funciones para la Lógica de riego ---*/

  // Inicializar todos los pines y componentes
  static void init();

  // Leer datos de sensores
  static int readSoilMoisture(int pinSensor);
  static int readLightIntensity(int pinSensor);
  static float readAirHumidity(void);
  static float readAirTemperature(void);
  static float readWaterLevel(void);

  // Funciones adicionales
  void readAllSensors(void);
  void clearAllReadings(void);
  String currentHour(void);
  static void saveDataInSD(const String &data);
  String createJSON(void);
  void changeConfigurationParameters(ChangeConfiguration newConfig);

  // Funciones para condicionales de riego
  bool isManualIrrigationActivated(void);
  bool isTimerIrrigationActivated(void);
  bool evaluateIrrigationDecision(void);
  bool evaluateIfIsTimeToWater(void);

  // Funciones para control y seguridad del riego
  bool isWaterLevelSafe(void);
  void driveRelay(bool activate);
  bool isRelayActive(void);
  void updateIrrigation(void);
};

void IrrigationControl ::init(void)
{
  // Inicialización de componentes
  lcd.init();
  lcd.backlight();
  lcd.print("Iniciando...");

  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(SOIL_MOISTURE1_PIN, INPUT);
  pinMode(SOIL_MOISTURE2_PIN, INPUT);
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(BTN_PIN1, INPUT_PULLUP);

  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);

  dht.begin();
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  if (!SD.begin(SD_CS))
  {
    Serial.println("Error inicializando tarjeta SD");
    while (true)
      ;
  }

  if (!rtc.begin())
  {
    Serial.println("Error inicializando RTC");
    while (true)
      ;
  }

  if (!rtc.isrunning())
  {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  lcd.clear();
  lcd.print("Sistema Listo");
  delay(2000);
}

void IrrigationControl ::changeConfigurationParameters(ChangeConfiguration newConfig)
{
  irrigationAtTime = newConfig.setIrrigationTime;
  minLightThreshold = newConfig.setLightThreshold;
  minSoilMoistureThreshold = newConfig.setSoilMoistureThreshold;
  manualIrrigationActivated = newConfig.setManualIrrigationMode;
  timerIrrigationActivated = newConfig.setTimerIrrigationMode;
  irrigationStatus = newConfig.setIrrigationStatus;
}
String IrrigationControl ::currentHour(void)
{
  return String(currentDate.hour()) + ":" + String(currentDate.minute());
}
/*-- Funciones para condicionales de riego --*/
bool IrrigationControl ::isManualIrrigationActivated(void)
{
  return manualIrrigationActivated;
}
bool IrrigationControl ::isTimerIrrigationActivated(void)
{
  return timerIrrigationActivated;
}

// Determinar riego por valores de lectura en los sensores
bool IrrigationControl ::evaluateIrrigationDecision(void)
{
  int medianSoilMoisture = (s_soilMoisture1 + s_soilMoisture2) / 2;

  return s_lightIntensity < minLightThreshold && medianSoilMoisture < minSoilMoistureThreshold;
}
bool IrrigationControl ::evaluateIfIsTimeToWater(void)
{
  if (currentHour() == irrigationAtTime && counterForTimer == 0)
  {
    counterForTimer++;
    return true;
  }
  else if (currentHour() != irrigationAtTime)
  {
    counterForTimer = 0;
  }
  return false;
}

/*-- Funciones para control y seguridad del riego --*/
bool IrrigationControl ::isWaterLevelSafe(void)
{
  return s_waterLevel >= 20;
}

void IrrigationControl ::driveRelay(bool activate)
{
  s_relayActive = activate;
  digitalWrite(RELAY1_PIN, activate ? HIGH : LOW);
}

bool IrrigationControl ::isRelayActive(void)
{
  return s_relayActive;
}

void IrrigationControl ::updateIrrigation(void)
{
  // Botón manual de bomba: solo aplica en modo manual (con antirrebote)
  if (manualIrrigationActivated && !digitalRead(BTN_PIN1))
  {
    if (millis() - lastPumpButtonPress > 300)
    {
      irrigationStatus = !irrigationStatus;
      lastPumpButtonPress = millis();
    }
  }

  // Condición base: nivel de agua insuficiente bloquea el riego sin importar el modo
  if (!isWaterLevelSafe())
  {
    driveRelay(false);
    return;
  }

  if (manualIrrigationActivated)
  {
    driveRelay(irrigationStatus);
  }
  else if (timerIrrigationActivated)
  {
    if (evaluateIfIsTimeToWater())
    {
      driveRelay(true);
    }
  }
  else
  {
    driveRelay(evaluateIrrigationDecision());
  }
}

/*-- Funciones para JSON y Memoria SD --*/

String IrrigationControl ::createJSON(void)
{
  // Crear JSON con estructura deseada
  DynamicJsonDocument doc(512);
  doc["fecha"] = String(currentDate.day()) + "/" + String(currentDate.month()) + "/" + String(currentDate.year());
  doc["hora"] = String(currentDate.hour()) + ":" + String(currentDate.minute()) + ":" + String(currentDate.second());
  doc["temperaturaAmbiente"] = s_airTemperature;
  doc["humedadAmbiente"] = s_airHumidity;
  doc["humedadSuelo"]["sensor1"] = s_soilMoisture1;
  doc["humedadSuelo"]["sensor2"] = s_soilMoisture2;
  doc["iluminacion"] = s_lightIntensity;
  doc["riegoManual"] = isRelayActive();
  doc["nivelAgua"] = s_waterLevel;

  // Convertir JSON a cadena
  String jsonString;
  serializeJson(doc, jsonString);

  return jsonString;
}

void IrrigationControl ::saveDataInSD(const String &data)
{
  // Abrir el archivo en modo escritura/apéndice
  File file = SD.open("/datalog.txt", FILE_APPEND);
  if (file)
  {
    file.println(data); // Escribir datos en el archivo
    file.close();       // Cerrar el archivo
    Serial.println("Datos guardados en datalog.txt:");
    Serial.println(data);
  }
  else
  {
    Serial.println("Error al abrir datalog.txt para escritura.");
  }
}

/* Funciones para lecturas de los sensores */
void IrrigationControl ::readAllSensors(void)
{
  s_lightIntensity = readLightIntensity(LDR_PIN);
  s_soilMoisture1 = readSoilMoisture(SOIL_MOISTURE1_PIN);
  s_soilMoisture2 = readSoilMoisture(SOIL_MOISTURE2_PIN);
  s_airTemperature = readAirTemperature();
  s_airHumidity = readAirHumidity();
  s_waterLevel = readWaterLevel();
  currentDate = rtc.now();
}

void IrrigationControl ::clearAllReadings(void)
{
  s_lightIntensity = 0;
  s_soilMoisture1 = 0;
  s_soilMoisture2 = 0;
  s_airTemperature = 0;
  s_airHumidity = 0;
  s_waterLevel = 0;
}

float IrrigationControl ::readAirHumidity(void)
{
  // Código para obtener la humedad del ambiente
  return dht.readHumidity();
}

float IrrigationControl ::readAirTemperature(void)
{
  // Código para obtener la temperatura del ambiente
  return dht.readTemperature();
}

int IrrigationControl ::readSoilMoisture(int pinSensor)
{
  // Código para obtener la humedad del suelo
  // map(soilMoistureValue, AirValue, WaterValue, 0, 100) <- Cambiar por nuestras lecturas
  return map(analogRead(pinSensor), 790, 390, 0, 100);
}

float IrrigationControl ::readWaterLevel(void)
{
  // Código para obtener el nivel del agua
  digitalWrite(TRIGGER, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIGGER, LOW);

  float distanceCm = pulseIn(ECHO, HIGH) * VELOCIDAD_SONIDO / 2;
  // Calibración del tanque: 17cm = vacío, 6cm = lleno (recalibrar si el tanque físico difiere)
  return map(distanceCm, 17, 6, 0, 100);
}

int IrrigationControl ::readLightIntensity(int pinSensor)
{
  // map(fotoresistencia, minReading, maxReading, minValue, maxValue);
  return map(analogRead(pinSensor), 0, 1000, 0, 100);
}

#endif
