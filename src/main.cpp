#include <WiFi.h>
#include <WebSocketsClient.h>
#include <Ticker.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

const char* ssid = "TP-Link_5648";
const char* password = "12345678";

// Configuración del WebSocket
const char* host = "192.168.1.104";
const int port = 5000;
const char* path = "/echo";

WebSocketsClient webSocketClient;

// Estructura para almacenar el estado de los botones
struct ButtonState {
    bool security;
    bool opendoor;
    bool offlights;
    bool alarm;
    bool room;
    bool dinning;
    bool bathroom;
    bool yarn;
    bool closeDoor;
};

// Inicializar el estado de los botones
ButtonState buttonsState = {false, false, false, false, false, false, false, false, false};
Servo servo;

// Definir los pines GPIO para cada botón
const int securityPin = 23;
const int opendoorPin = 22;
const int offlightsPin = 21;
const int alarmPin = 19;
const int roomPin = 18;
const int dinningPin = 5;
const int bathroomPin = 4;
const int yarnPin = 16;
const int closeDoorPin = 2;
const int end_of_race_door= 34;

// Función para configurar los pines GPIO
void setupPins() {
    pinMode(securityPin, OUTPUT);
    pinMode(opendoorPin, OUTPUT);
    pinMode(offlightsPin, OUTPUT);
    pinMode(alarmPin, OUTPUT);
    pinMode(roomPin, OUTPUT);
    pinMode(dinningPin, OUTPUT);
    pinMode(bathroomPin, OUTPUT);
    pinMode(yarnPin, OUTPUT);
    pinMode(end_of_race_door, INPUT_PULLDOWN);
    servo.attach(closeDoorPin, 500, 2500);
}


// Función para enviar el estado actualizado de los botones al servidor de Node.js
void sendUpdatedState() {
    DynamicJsonDocument doc(256);
    doc["security"] = buttonsState.security;
    doc["opendoor"] = buttonsState.opendoor;
    doc["offlights"] = buttonsState.offlights;
    doc["alarm"] = buttonsState.alarm;
    doc["room"] = buttonsState.room;
    doc["dinning"] = buttonsState.dinning;
    doc["bathroom"] = buttonsState.bathroom;
    doc["yarn"] = buttonsState.yarn;
    doc["closeDoor"] = buttonsState.closeDoor;

    String output;
    serializeJson(doc, output);
    webSocketClient.sendTXT(output);
}

int check = 0;
// Función para actualizar los pines GPIO según el estado de los botones
void updatePins() {
    digitalWrite(securityPin, buttonsState.security ? HIGH : LOW);
    digitalWrite(opendoorPin, buttonsState.opendoor ? HIGH : LOW);
    digitalWrite(offlightsPin, buttonsState.offlights ? HIGH : LOW);
    digitalWrite(alarmPin, buttonsState.alarm ? HIGH : LOW);
    digitalWrite(roomPin, buttonsState.room ? HIGH : LOW);
    digitalWrite(dinningPin, buttonsState.dinning ? HIGH : LOW);
    digitalWrite(bathroomPin, buttonsState.bathroom ? HIGH : LOW);
    digitalWrite(yarnPin, buttonsState.yarn ? HIGH : LOW);

    int pinState= digitalRead(end_of_race_door);
    
    if (pinState==HIGH && check==0)
    {
        check=1;
        buttonsState.closeDoor = false;
        sendUpdatedState();
        delay(200);
    }
    

    if (pinState==LOW)
    {
        check=0;
        buttonsState.closeDoor = true;
        sendUpdatedState();
    }
    
    if (buttonsState.closeDoor && pinState==HIGH) {
        check=0;
        buttonsState.closeDoor = false;
        sendUpdatedState();
        servo.write(0);
        delay(1500);
        servo.write(180);
        delay(1500);
    }
}

// Función de callback para recibir mensajes
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    DynamicJsonDocument doc(256); // Declarar esto antes del switch statement
    DeserializationError error; // Declarar esto antes del switch statement

    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("Desconectado!");
            break;
        case WStype_CONNECTED:
            Serial.println("Conectado!");
            break;
        case WStype_TEXT:
            Serial.printf("Mensaje recibido: %s\n", payload);
            // Parsear el JSON payload
            error = deserializeJson(doc, payload);
            if (!error) {
                // Actualizar el estado de los botones
                buttonsState.security = doc["security"];
                buttonsState.opendoor = doc["opendoor"];
                buttonsState.offlights = doc["offlights"];
                buttonsState.alarm = doc["alarm"];
                buttonsState.room = doc["room"];
                buttonsState.dinning = doc["dinning"];
                buttonsState.bathroom = doc["bathroom"];
                buttonsState.yarn = doc["yarn"];
                buttonsState.closeDoor = doc["closeDoor"];

                // Imprimir el estado de cada botón
                Serial.printf("security: %s\n", buttonsState.security ? "true" : "false");
                Serial.printf("opendoor: %s\n", buttonsState.opendoor ? "true" : "false");
                Serial.printf("offlights: %s\n", buttonsState.offlights ? "true" : "false");
                Serial.printf("alarm: %s\n", buttonsState.alarm ? "true" : "false");
                Serial.printf("room: %s\n", buttonsState.room ? "true" : "false");
                Serial.printf("dinning: %s\n", buttonsState.dinning ? "true" : "false");
                Serial.printf("bathroom: %s\n", buttonsState.bathroom ? "true" : "false");
                Serial.printf("yarn: %s\n", buttonsState.yarn ? "true" : "false");
                Serial.printf("closeDoor: %s\n", buttonsState.closeDoor ? "true" : "false");
                // Actualizar los pines GPIO según el estado de los botones
                updatePins();
            } else {
                Serial.println("Error parsing JSON!");
            }
            break;
        case WStype_BIN:
            Serial.println("Mensaje binario recibido");
            break;
    }
}

void connectToWiFi() {
  Serial.print("Conectando a ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

void connectToWebSocket() {
  Serial.print("Conectando a WebSocket en ");
  Serial.println(host);
  
  webSocketClient.begin(host, port, path);
  webSocketClient.onEvent(webSocketEvent);
}

void setup() {
  Serial.begin(115200);
  connectToWiFi();
  connectToWebSocket();
  setupPins();
}

void loop() {
  webSocketClient.loop();
}
