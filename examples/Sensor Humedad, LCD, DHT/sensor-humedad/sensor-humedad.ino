#include <DHTesp.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
DHTesp dht;
TempAndHumidity data;
#define DHT_PIN 16
#define ldrPin 34
#define buttonPin 2

int ldrValue;
int currentScreen = 0; // Valor que gestiona del 0 al 2 las pantallas

void setup() {
  Serial.begin(115200);
  pinMode(ldrPin, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);  // Botón con resistencia pull-up

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);  // Detener ejecución si falla la inicialización
  }

  display.clearDisplay();
  display.display();
  dht_init();
}

void loop() {
  readSensorData();
  displayScreen();
  handleButtonPress();
}

void dht_init() {
  dht.setup(DHT_PIN, DHTesp::DHT11);  // Configuración para DHT11
}

void handleButtonPress() {
  if (digitalRead(buttonPin) == LOW) {
    currentScreen++;
    if (currentScreen > 2) {
      currentScreen = 0;  // Reiniciar a 0 si es mayor a 2
    }
    Serial.println(currentScreen);
    delay(200);
  }
}

void readSensorData() {
  ldrValue = analogRead(ldrPin);
  data = dht.getTempAndHumidity();
}

void displayScreen() {
  display.clearDisplay();
  display.setTextSize(1);  
  display.setTextColor(WHITE);   

  switch (currentScreen) {
    case 0:
      display.setCursor(0, 0);  // Primera línea
      display.println("Diego Flores");
      break;
    case 1:
      display.setCursor(0, 0);  // Primera línea
      display.print("Temp: ");
      Serial.println(data.temperature);
      display.print(data.temperature);
      display.println(" C");

      display.setCursor(0, 10);  // Segunda línea
      display.print("Humidity: ");
      Serial.println(data.humidity);
      display.print(data.humidity);
      display.println(" %");
      break;

    case 2:
      display.setCursor(0, 0);
      display.print("LDR Value: ");
      display.println(ldrValue);
      break;
    }

    display.display();
}