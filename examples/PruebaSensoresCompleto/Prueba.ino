#include <SD.h>                  // Librería para el módulo SD
#include <Wire.h>                // Librería para I2C
#include <LiquidCrystal_I2C.h>   // Librería para pantalla LCD
#include <DHT.h>                 // Librería para sensor de temperatura y humedad
#include <RTClib.h>              // Librería para RTC

// Pines de conexión
const int SD_PIN = 5;
const int ldrPin = 35;
const int button1Pin = 14;
const int button2Pin = 12;
const int soilMoisture1Pin = 34;
const int soilMoisture2Pin = 33;
const int relay1Pin = 32;
const int relay2Pin = 27;
const int dhtPin = 16;

// Configuración de los sensores y dispositivos
DHT dht(dhtPin, DHT11);               // Sensor DHT11 (puede cambiar a DHT22 si lo usas)
LiquidCrystal_I2C lcd(0x27, 16, 2);   // Dirección I2C de la LCD (ajusta si es necesario)
RTC_DS1307 rtc;                       // RTC DS1307

void setup() {
  Serial.begin(115200);

  // Inicialización del módulo SD
  if (!SD.begin(SD_PIN)) {
    Serial.println("Error al inicializar el módulo SD.");
  } else {
    Serial.println("Módulo SD inicializado correctamente.");
  }

  // Inicialización de la pantalla LCD
  lcd.init();
  lcd.backlight();
  lcd.print("Iniciando...");

  // Inicialización de los botones
  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);

  // Inicialización de los relevadores
  pinMode(relay1Pin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);

  // Inicialización del sensor de temperatura y humedad
  dht.begin();

  // Inicialización del RTC
  if (!rtc.begin()) {
    Serial.println("Error al inicializar el RTC.");
  } else {
    Serial.println("RTC inicializado correctamente.");
    if (!rtc.isrunning()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Ajusta la hora con la compilación actual
    }
  }

  // Mensaje inicial en LCD
  lcd.clear();
  lcd.print("Sistema Listo");
  delay(2000);
}

void loop() {
  // Lectura del LDR
  int ldrValue = analogRead(ldrPin);
  Serial.print("Luz (LDR): ");
  Serial.println(ldrValue);

  // Lectura de los botones
  if (digitalRead(button1Pin) == LOW) {
    lcd.clear();
    lcd.print("Boton 1 Presionado");
  } else if (digitalRead(button2Pin) == LOW) {
    lcd.clear();
    lcd.print("Boton 2 Presionado");
  }

  // Lectura de los sensores de humedad de suelo
  int soilMoisture1 = analogRead(soilMoisture1Pin);
  int soilMoisture2 = analogRead(soilMoisture2Pin);
  Serial.print("Humedad Suelo 1: ");
  Serial.println(soilMoisture1);
  Serial.print("Humedad Suelo 2: ");
  Serial.println(soilMoisture2);

  // Lectura del sensor DHT
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Error al leer el sensor DHT.");
  } else {
    Serial.print("Temperatura: ");
    Serial.print(temperature);
    Serial.print(" °C, Humedad: ");
    Serial.print(humidity);
    Serial.println(" %");
  }

  // Lectura de la hora actual desde el RTC
  DateTime now = rtc.now();
  Serial.print("Fecha y Hora: ");
  Serial.print(now.day(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.year(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.println(now.second(), DEC);

  // Mostrar datos en LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print(" C");
  lcd.setCursor(0, 1);
  lcd.print("Hum: ");
  lcd.print(humidity);
  lcd.print(" %");
  delay(2000);

  // Blink de los relevadores
  digitalWrite(relay1Pin, HIGH);
  delay(500);
  digitalWrite(relay1Pin, LOW);
  digitalWrite(relay2Pin, HIGH);
  delay(500);
  digitalWrite(relay2Pin, LOW);

  // Guardar lectura en el módulo SD
  File dataFile = SD.open("/datalog.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.print("Fecha y Hora: ");
    dataFile.print(now.day(), DEC);
    dataFile.print('/');
    dataFile.print(now.month(), DEC);
    dataFile.print('/');
    dataFile.print(now.year(), DEC);
    dataFile.print(" ");
    dataFile.print(now.hour(), DEC);
    dataFile.print(':');
    dataFile.print(now.minute(), DEC);
    dataFile.print(':');
    dataFile.print(now.second(), DEC);
    dataFile.print(", Luz (LDR): ");
    dataFile.print(ldrValue);
    dataFile.print(", Temp: ");
    dataFile.print(temperature);
    dataFile.print(", Humedad: ");
    dataFile.print(humidity);
    dataFile.print(", Humedad Suelo 1: ");
    dataFile.print(soilMoisture1);
    dataFile.print(", Humedad Suelo 2: ");
    dataFile.println(soilMoisture2);
    dataFile.close();
    Serial.println("Datos guardados en SD.");
  } else {
    Serial.println("Error al escribir en la SD.");
  }

  delay(1000); // Espera antes de la siguiente lectura
}
