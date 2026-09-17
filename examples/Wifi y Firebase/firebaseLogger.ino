#include <WiFi.h>
#include <Firebase.h>
#include <ArduinoJson.h>

#define REFERENCE_URL "https://iot-srm-2024-default-rtdb.firebaseio.com/"

const char* ssid = "[...]";
const char* password = "[...]";

WiFiClient espClient;

Firebase fb(REFERENCE_URL);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  setup_wifi();
}

void loop() {
  writeDemoJSON();
  delay(10000);
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void writeDemoJSON(void){
  // Create a JSON document to hold the output data
  JsonDocument docOutput;

  // Add various data types to the JSON document
  docOutput["myString"] = "Joto el que lo lea";
  docOutput["exception"] = "Menos el creador";
  docOutput["myInt"] = 123;
  docOutput["myFloat"] = 45.67;
  docOutput["myBool"] = true;

  // Create a string to hold the serialized JSON data
  String output;

  // Optional: Shrink the JSON document to fit its contents exactly
  docOutput.shrinkToFit();

  // Serialize the JSON document to a string
  serializeJson(docOutput, output);

  // Set the serialized JSON data in Firebase
  if(!fb.pushJson("prueba/", output)){
    Serial.println("Error al enviar JSON");
  }
}