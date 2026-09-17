#ifndef WiFiMQTT_h
#define WiFiMQTT_h

#include <WiFi.h>
#include <PubSubClient.h>
#include "env.h"

// Crear un archivo llamado env.h con los valores y agregarlo:
// struct KeysEnv {
// 	char* ssid = "[ssid]";
//   char* password =  "[pswd]";
// 	char* mqtt_server = "[server]";
// } KeysEnv;
struct KeysEnv env;

const char *ssid = env.ssid;
const char *password = env.password;
const char *mqtt_server = env.mqtt_server;
const uint16_t mqtt_port = env.MQTT_PORT;

// Conexiones
WiFiClient espClient;
PubSubClient mqttClient(espClient);

class WifiMqtt
{
public:
  static void startConnections(void);
  static void connectWiFi(void);
  static void reconnectWiFi(void);
  static bool isWiFiConnected(void);
  static void connectMQTT(void);
  static void reconnectMQTT(void);
  static bool isMQTTConnected(void);
  static void publishMessage(const char *payload);
  static void subscribeTopic(char *topic);
};

void WifiMqtt ::startConnections(void)
{
  connectWiFi();
  connectMQTT();
  Serial.println("Iniciando conexiones MQTT y WiFI");
}

void WifiMqtt ::connectWiFi(void)
{
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (!isWiFiConnected())
  {
    // Serial.print(".");
    delay(1);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

bool WifiMqtt ::isWiFiConnected(void)
{
  return WiFi.status() == WL_CONNECTED;
}

void WifiMqtt ::reconnectWiFi(void)
{
  WiFi.disconnect();
  if (!isWiFiConnected())
  {
    Serial.print("Reconectando a WiFi...");
    connectWiFi();
    delay(1);
  }
}

void WifiMqtt ::connectMQTT(void)
{
  mqttClient.setServer(mqtt_server, mqtt_port);
}

bool WifiMqtt ::isMQTTConnected(void)
{
  return mqttClient.connected();
}

void WifiMqtt ::reconnectMQTT(void)
{
  Serial.print("Intentando conectar a MQTT...");
  if (mqttClient.connect("ucol/iot"))
  {
    Serial.println("connected");
    mqttClient.subscribe(env.topicRX);
    Serial.println("Suscrito al topic ucol/iot/config");
  }
  else
  {
    Serial.print("Error de conexión MQTT: , rc=");
    Serial.print(mqttClient.state());
    connectMQTT();
  }
}

// MODIFICAR FUNCION PARA QUE SEA CON ENV O MARCAR UN DEFAULT DEL TOPICO DE ENVÍO DE DATOS
void WifiMqtt ::publishMessage(const char *payload)
{
  if (isMQTTConnected())
  {
    mqttClient.publish(env.topicTX, payload);
    Serial.print("Mensaje publicado en el topic ");
    Serial.print(env.topicTX);
    Serial.print(": ");
    Serial.println(payload);
  }
  else
  {
    Serial.println("No se puede publicar. MQTT no está conectado.");
  }
}

void WifiMqtt ::subscribeTopic(char *topic)
{
  mqttClient.subscribe(topic);
}

#endif
