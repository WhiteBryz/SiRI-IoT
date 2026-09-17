#ifndef DualCore_h
#define DualCore_h

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "WiFiMQTT.h"
#include "IrrigationControl.h"

// Claves de los núcleos
#define NUCLEO_PRIMARIO 0X01
#define NUCLEO_SECUNDARIO 0X00

// Definición del intervalo de lectura en milisegundos
#define SENSOR_READ_INTERVAL 5000  // 5 segundos

struct SensorsData {
  float temperature;
  float himidity;
  float soilMoisture;
  uint32_t timestamp;
};

struct MQTTMessage {
    char message[256];  // Ajusta el tamaño según tus necesidades
};

WifiMqtt Wireless;
IrrigationControl iCtrl;

class DualCoreESP32{
  public:
    void ConfigCores( void ); // Creación de tareas xTaskCreatePinnedToCore

  private:

    // Tareas primer núcleo
    TaskHandle_t WiFiMQTTTask_t;

    // Tareas segundo núcleo
    TaskHandle_t ReadSensorsTask_t;

    // Queues
    static QueueHandle_t mqttQueue;

    static void WiFiMQTTTask( void * pvParameters );
    static void ReadSensorsTask( void *pvParameters );
};

// Inicializar la cola estática
QueueHandle_t DualCoreESP32::mqttQueue = NULL;

void DualCoreESP32 :: ConfigCores( void ){
  // Inicializar colas
  mqttQueue = xQueueCreate(10, sizeof(MQTTMessage));

  Serial.println("Entro a ConfigCores");

  // Conexión a Wifi y MQTT
  xTaskCreatePinnedToCore(
    this->WiFiMQTTTask,
    "WirelessConnections",
    10000,
    NULL,
    1,
    &WiFiMQTTTask_t,
    NUCLEO_PRIMARIO
  );

  // No hay tareas separadas de envío/recepción MQTT: WiFiMQTTTask ya desencola mqttQueue
  // y publica (ver abajo), y PubSubClient invoca mqttCallback de forma síncrona dentro de
  // mqttClient.loop() (también llamado en cada iteración de WiFiMQTTTask).

  // Leer sensores y generar el JSON
  xTaskCreatePinnedToCore(
    this->ReadSensorsTask,
    "ReadSensors",
    10000,
    NULL,
    1,
    &ReadSensorsTask_t,
    NUCLEO_SECUNDARIO
  );

}

// Callback MQTT: parsea el JSON entrante y actualiza la configuración de riego
void mqttCallback(char* topic, byte* payload, unsigned int length){
  String message;
  for (unsigned int i = 0; i < length; i++){
    message += (char)payload[i];
  }

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);
  if (error){
    Serial.println("Error al parsear JSON recibido por MQTT");
    return;
  }

  iCtrl.changeConfigurationParameters(doc);
}

void DualCoreESP32 :: WiFiMQTTTask( void * pvParameters ){
  Serial.println("Entro a WiFiMQTTTask");
  Wireless.startConnections();
  mqttClient.setCallback(mqttCallback);

  // Buffer para recibir mensajes de la cola
  MQTTMessage receivedMessage;

  while(true){
    if(!Wireless.isWiFiConnected()){
      Serial.println("WiFi Desconectado");
      Wireless.reconnectWiFi();
    } else{
      if(!Wireless.isMQTTConnected()){
        Serial.println("MQTT Desconectado");
        Wireless.reconnectMQTT();
      } else {
        // Verificar si hay mensajes para publicar en la cola
        if(xQueueReceive(mqttQueue, &receivedMessage, 0) == pdTRUE) {
            // Publicar el mensaje si hay conexión MQTT
            Wireless.publishMessage(receivedMessage.message);
        }
      }
    //  Serial.println("Todo bien");
    }
    mqttClient.loop();
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}

void DualCoreESP32 :: ReadSensorsTask ( void * pvParameters){
  // Inicializar controlador de riego
  iCtrl.init();

  // Variable para almacenar el tiempo de la última lectura
  unsigned long lastReadTime = 0;

  // Estructura para mensaje MQTT
  MQTTMessage mqttMessage;

  while(true){
    // Obtener el tiempo actual
    unsigned long currentTime = millis();

    if(currentTime - lastReadTime >= SENSOR_READ_INTERVAL){
      lastReadTime = currentTime;

      // Realizar la lectura de sensores
      iCtrl.readAllSensors();

      // Crear y guardar JSON
      String json = iCtrl.createJSON();
      iCtrl.saveDataInSD(json);

      // Copiar el JSON al mensaje MQTT
      strncpy(mqttMessage.message, json.c_str(), sizeof(mqttMessage.message) - 1);
      mqttMessage.message[sizeof(mqttMessage.message) - 1] = '\0';  // Asegurar terminación null

      // Enviar a la cola MQTT
      xQueueSend(mqttQueue, &mqttMessage, 0);
    }

    // Control de riego: debe correr en cada iteración, no solo cada SENSOR_READ_INTERVAL
    iCtrl.updateIrrigation();

    vTaskDelay(100/portTICK_PERIOD_MS);
  }
}
#endif