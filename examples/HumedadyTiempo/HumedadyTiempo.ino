#include <Wire.h>
#include <RTClib.h>

#define sensorPin 34  // Pin analógico para el sensor de humedad

RTC_DS3231 rtc;
int humedad = 0;

void setup()
{
    Serial.begin(115200);
    Wire.begin(21, 22);  // SDA y SCL del ESP32

    // Inicializar el RTC
    if (!rtc.begin()) {
        Serial.println("No se encontró RTC");
        while (1);
    }

    // Ajustar la fecha y hora si el RTC perdió energía
    if (rtc.lostPower()) {
        Serial.println("RTC perdido, estableciendo la fecha y hora...");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
}

void loop()
{
    // Leer el sensor de humedad y convertir a porcentaje
    humedad = map(analogRead(sensorPin), 0, 4095, 100, 0);
    Serial.print("Humedad: ");
    Serial.print(humedad);
    Serial.println("%");

    // Leer y mostrar la fecha y hora del RTC
    DateTime now = rtc.now();
    Serial.print("Fecha y hora: ");
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();

    delay(1000);
}
