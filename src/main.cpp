#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

const char* ssid = "TP-Link_5648";
const char* password = "12345678";
const char* websocket_server_host = "192.168.1.103"; // IP del servidor Express
const uint16_t websocket_server_port = 5000; // Puerto del servidor Express

WebSocketsClient webSocket;

// Define los pines que vas a controlar según el pinout de la imagen
const int securityPin = 2; // Ajusta este pin según tu necesidad
const int opendoorPin = 4; // Ajusta este pin según tu necesidad
const int offlightsPin = 5; // Ajusta este pin según tu necesidad
const int alarmPin = 18; // Ajusta este pin según tu necesidad
const int roomPin = 19; // Ajusta este pin según tu necesidad
const int dinningPin = 21; // Ajusta este pin según tu necesidad
const int bathroomPin = 22; // Ajusta este pin según tu necesidad
const int yarnPin = 23; // Ajusta este pin según tu necesidad

// Declaración de las funciones
void onWebSocketEvent(WStype_t type, uint8_t *payload, size_t length);
void handleButtonState(String payload);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  // Conectar al WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a WiFi...");
  }
  
  Serial.println("Conectado a WiFi");
  
  // Inicializa los pines como salidas
  pinMode(securityPin, OUTPUT);
  pinMode(opendoorPin, OUTPUT);
  pinMode(offlightsPin, OUTPUT);
  pinMode(alarmPin, OUTPUT);
  pinMode(roomPin, OUTPUT);
  pinMode(dinningPin, OUTPUT);
  pinMode(bathroomPin, OUTPUT);
  pinMode(yarnPin, OUTPUT);

  // Conectar al servidor WebSocket
  webSocket.begin(websocket_server_host, websocket_server_port, "/");

  // Llamar a la función onWebSocketEvent cuando ocurra un evento
  webSocket.onEvent(onWebSocketEvent);
  
  // Intentar la reconexión en caso de desconexión
  webSocket.setReconnectInterval(5000);
}

void loop() {
  webSocket.loop();
}

void onWebSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("Desconectado del servidor WebSocket");
      break;
    case WStype_CONNECTED: {
      Serial.println("Conectado al servidor WebSocket");
      // Enviar mensaje en formato JSON cuando se conecte
      DynamicJsonDocument doc(200);
      doc["message"] = "Hello server!";
      String output;
      serializeJson(doc, output);
      webSocket.sendTXT(output);
      break;
    }
    case WStype_TEXT: {
      Serial.printf("Mensaje recibido: %s\n", payload);
      // Procesa el mensaje recibido
      handleButtonState(String((char*)payload));
      break;
    }
  }
}

void handleButtonState(String payload) {
  // Parsea el JSON recibido
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print(F("Error parsing JSON: "));
    Serial.println(error.c_str());
    return;
  }

  // Actualiza los estados de los botones y controla los pines correspondientes
  if (doc.containsKey("security")) {
    digitalWrite(securityPin, doc["security"] ? HIGH : LOW);
    Serial.println("ledsecurity");
  }
  if (doc.containsKey("opendoor")) {
    digitalWrite(opendoorPin, doc["opendoor"] ? HIGH : LOW);
    Serial.println("opendoor");
  }
  if (doc.containsKey("offlights")) {
    digitalWrite(offlightsPin, doc["offlights"] ? HIGH : LOW);
    Serial.println("offlights");
  }
  if (doc.containsKey("alarm")) {
    digitalWrite(alarmPin, doc["alarm"] ? HIGH : LOW);
    Serial.println("alarm");
  }
  if (doc.containsKey("room")) {
    digitalWrite(roomPin, doc["room"] ? HIGH : LOW);
    Serial.println("room");
  }
  if (doc.containsKey("dinning")) {
    digitalWrite(dinningPin, doc["dinning"] ? HIGH : LOW);
    Serial.println("dinning");
  }
  if (doc.containsKey("bathroom")) {
    digitalWrite(bathroomPin, doc["bathroom"] ? HIGH : LOW);
    Serial.println("bathroom");
  }
  if (doc.containsKey("yarn")) {
    digitalWrite(yarnPin, doc["yarn"] ? HIGH : LOW);
    Serial.println("yarn");
  }
}
