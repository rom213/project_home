#include <WiFi.h>
#include <WebSocketsClient.h>
#include <Ticker.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

const char *ssid = "TP-Link_5648";
const char *password = "12345678";

// Configuración del WebSocket
const char *host = "192.168.1.108";
const int port = 5000;
const char *path = "/echo";
const int port2 = 5001;
WebSocketsClient webSocketClient;

// Estructura para almacenar el estado de los botones
struct ButtonState
{
    bool security;
    bool signal_close_door;
    bool opendoor;
    bool offlights;
    bool alarm;
    bool room;
    bool dinning;
    bool bathroom;
    bool yarn;
    bool closeDoor;
    bool ethernet;
    bool ethernet_state;
};

// Inicializar el estado de los botones
ButtonState buttonsState = {false, false, false, false, false, false, false, false, false, false, true, true};
Servo servo;

// Estatus del ethernet
int status_ethernet = false;

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
const int end_of_race_door = 34;
const int on_off_ethernet = 27;
const int alarm_detected_pin = 14;

// Función para configurar los pines GPIO
void setupPins()
{
    pinMode(securityPin, OUTPUT);
    pinMode(opendoorPin, OUTPUT);
    pinMode(offlightsPin, OUTPUT);
    pinMode(alarmPin, OUTPUT);
    pinMode(roomPin, OUTPUT);
    pinMode(dinningPin, OUTPUT);
    pinMode(bathroomPin, OUTPUT);
    pinMode(yarnPin, OUTPUT);
    pinMode(end_of_race_door, INPUT_PULLDOWN);
    pinMode(on_off_ethernet, INPUT_PULLDOWN);
    pinMode(alarm_detected_pin, INPUT_PULLDOWN);
    servo.attach(closeDoorPin, 500, 2500);
}

// Función para enviar el estado actualizado de los botones al servidor de Node.js
void sendUpdatedState()
{

    if (WiFi.status() == WL_CONNECTED && status_ethernet == true)
    {
        DynamicJsonDocument doc(256);
        doc["security"] = buttonsState.security;
        doc["opendoor"] = buttonsState.opendoor;
        doc["offlights"] = buttonsState.offlights;
        doc["alarm"] = buttonsState.alarm;
        doc["room"] = buttonsState.room;
        doc["dinning"] = buttonsState.dinning;
        doc["signal_close_door"] = buttonsState.signal_close_door;
        doc["bathroom"] = buttonsState.bathroom;
        doc["yarn"] = buttonsState.yarn;
        doc["closeDoor"] = buttonsState.closeDoor;
        doc["ethernet"] = true;
        doc["ethernet_state"] = true;

        String output;
        serializeJson(doc, output);
        webSocketClient.sendTXT(output);
    }

    if (WiFi.status() == WL_CONNECTED && status_ethernet == false)
    {
        DynamicJsonDocument doc(256);
        doc["security"] = buttonsState.security;
        doc["opendoor"] = buttonsState.opendoor;
        doc["offlights"] = buttonsState.offlights;
        doc["alarm"] = buttonsState.alarm;
        doc["room"] = buttonsState.room;
        doc["dinning"] = buttonsState.dinning;
        doc["signal_close_door"] = buttonsState.signal_close_door;
        doc["bathroom"] = buttonsState.bathroom;
        doc["yarn"] = buttonsState.yarn;
        doc["closeDoor"] = buttonsState.closeDoor;
        doc["ethernet"] = false;
        doc["ethernet_state"] = true;

        String output;
        serializeJson(doc, output);
        webSocketClient.sendTXT(output);
    }
}


void sendCheckDoor()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        String url = "http://192.168.1.108:5001/socket/checkDoor";  // Ajusta según tu backend

        http.begin(url);
        http.addHeader("Content-Type", "application/json");  // Especificar JSON en el encabezado

        // Crear el JSON con el estado de la puerta
        String jsonPayload = "{\"closeDoor\":" + String(buttonsState.closeDoor ? "true" : "false") + "}";

        int httpCode = http.POST(jsonPayload);  // Enviar la solicitud POST

        http.end();  // Cierra la conexión
    }
}


int check = 0;
unsigned long lastCheckTime = 0;  // Variable para el temporizador


void checkDoor(){
    int pinState = digitalRead(end_of_race_door);

    if (status_ethernet == true)
    {
        if (pinState == HIGH && check == 0)
        {
            check = 1;
            buttonsState.closeDoor = false;
            sendCheckDoor();
            delay(200);
        }

        if (pinState == LOW)
        {
            check = 0;
            buttonsState.closeDoor = true;
            if (millis() - lastCheckTime >= 5000)
            {
                lastCheckTime = millis();
                sendCheckDoor();
            }
        }
    }
}

void closeDoor(){
    if (status_ethernet == true && buttonsState.signal_close_door)
    {
        Serial.println("hola se debe cerrar");
        buttonsState.signal_close_door= false;
        sendUpdatedState();
        servo.write(0);
        delay(1500);
        servo.write(180);
        delay(1500);
    }
}


// Función para actualizar los pines GPIO según el estado de los botones
void updatePins()
{
    digitalWrite(securityPin, buttonsState.security ? HIGH : LOW);
    digitalWrite(opendoorPin, buttonsState.opendoor ? HIGH : LOW);
    digitalWrite(offlightsPin, buttonsState.offlights ? HIGH : LOW);
    digitalWrite(alarmPin, buttonsState.alarm ? HIGH : LOW);
    digitalWrite(roomPin, buttonsState.room ? HIGH : LOW);
    digitalWrite(dinningPin, buttonsState.dinning ? HIGH : LOW);
    digitalWrite(bathroomPin, buttonsState.bathroom ? HIGH : LOW);
    digitalWrite(yarnPin, buttonsState.yarn ? HIGH : LOW);
}

// Función de callback para recibir mensajes
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
    if (WiFi.status() == WL_CONNECTED && status_ethernet == true)
    {
        DynamicJsonDocument doc(256); // Declarar esto antes del switch statement
        DeserializationError error;   // Declarar esto antes del switch statement

        switch (type)
        {
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
            if (!error)
            {
                // Actualizar el estado de los botones
                buttonsState.security = doc["security"];
                buttonsState.opendoor = doc["opendoor"];
                buttonsState.offlights = doc["offlights"];
                buttonsState.signal_close_door= doc["signal_close_door"];
                buttonsState.alarm = doc["alarm"];
                buttonsState.room = doc["room"];
                buttonsState.dinning = doc["dinning"];
                buttonsState.bathroom = doc["bathroom"];
                buttonsState.yarn = doc["yarn"];
                buttonsState.closeDoor = doc["closeDoor"];

                // Imprimir el estado de cada botón
                // Serial.printf("security: %s\n", buttonsState.security ? "true" : "false");
                // Serial.printf("opendoor: %s\n", buttonsState.opendoor ? "true" : "false");
                // Serial.printf("offlights: %s\n", buttonsState.offlights ? "true" : "false");
                // Serial.printf("alarm: %s\n", buttonsState.alarm ? "true" : "false");
                // Serial.printf("room: %s\n", buttonsState.room ? "true" : "false");
                // Serial.printf("dinning: %s\n", buttonsState.dinning ? "true" : "false");
                // Serial.printf("bathroom: %s\n", buttonsState.bathroom ? "true" : "false");
                // Serial.printf("yarn: %s\n", buttonsState.yarn ? "true" : "false");
                // Serial.printf("closeDoor: %s\n", buttonsState.closeDoor ? "true" : "false");
                // Actualizar los pines GPIO según el estado de los botones
                updatePins();
            }
            else
            {
                Serial.println("Error parsing JSON!");
            }
            break;
        case WStype_BIN:
            Serial.println("Mensaje binario recibido");
            break;
        }
    }
    if (WiFi.status() == WL_CONNECTED && status_ethernet == false)
    {

        DynamicJsonDocument doc(256); // Declarar esto antes del switch statement
        DeserializationError error;   // Declarar esto antes del switch statement

        switch (type)
        {
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
            if (!error)
            {
                if (doc["ethernet"])
                {
                    Serial.println("estado cambiado");
                    status_ethernet = true;

                    updatePins();
                }

                // Actualizar el estado de los botones

                // Imprimir el estado de cada botón
                // Serial.printf("security: %s\n", buttonsState.security ? "true" : "false");
                // Serial.printf("opendoor: %s\n", buttonsState.opendoor ? "true" : "false");
                // Serial.printf("offlights: %s\n", buttonsState.offlights ? "true" : "false");
                // Serial.printf("alarm: %s\n", buttonsState.alarm ? "true" : "false");
                // Serial.printf("room: %s\n", buttonsState.room ? "true" : "false");
                // Serial.printf("dinning: %s\n", buttonsState.dinning ? "true" : "false");
                // Serial.printf("bathroom: %s\n", buttonsState.bathroom ? "true" : "false");
                // Serial.printf("yarn: %s\n", buttonsState.yarn ? "true" : "false");
                // Serial.printf("closeDoor: %s\n", buttonsState.closeDoor ? "true" : "false");
                // Actualizar los pines GPIO según el estado de los botones
            }
            else
            {
                Serial.println("Error parsing JSON!");
            }
            break;
        case WStype_BIN:
            Serial.println("Mensaje binario recibido");
            break;
        }
    }
}

void connectToWiFi()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.print("Conectando a ");
        Serial.println(ssid);

        WiFi.begin(ssid, password);

        unsigned long tiempoInicio = millis();          // Marca de tiempo inicial
        const unsigned long tiempoMaximoEspera = 10000; // Máximo tiempo de espera (10 segundos)

        while (WiFi.status() != WL_CONNECTED)
        {

            if (millis() - tiempoInicio > tiempoMaximoEspera)
            { // Si supera el tiempo máximo, salir
                Serial.println("No se pudo conectar al WiFi dentro del tiempo límite.");
                return; // Sale de la función si no se conecta dentro del tiempo
            }

            delay(500); // Pausa breve para evitar saturar el bucle
            Serial.print(".");
        }

        // Si se conecta correctamente
        Serial.println("\nWiFi conectado");
        Serial.println("Dirección IP: ");
        Serial.println(WiFi.localIP());
    }
}
void connectToWebSocket()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.print("Conectando a WebSocket en ");
        Serial.println(host);

        webSocketClient.begin(host, port, path);
        webSocketClient.onEvent(webSocketEvent);
    }
}

void setup()
{
    Serial.begin(115200);
    setupPins();
    connectToWiFi();
    connectToWebSocket();
    sendUpdatedState();
}

bool status_render = true;

void ethernetControl()
{
    int pinStateEtherNet = digitalRead(on_off_ethernet);

    if (pinStateEtherNet == HIGH)
    {
        Serial.println("estado cambiado desde el boton");
        status_ethernet = !status_ethernet;
        delay(700);
    }

    if (status_ethernet == true && status_render == true)
    {
        buttonsState = {false, false, false ,false, false, false, false, false, false, false, false, true};
        updatePins();
        sendUpdatedState();
        status_render = false;
    }

    if (status_ethernet == false && status_render == false)
    {
        buttonsState = {false, false, false, false, false, false, false, false, false, false, false, true};
        updatePins();
        if (WiFi.status() == WL_CONNECTED)
        {
            sendUpdatedState();
        }

        status_render = true;
    }
}

void checkEthernetDisconnect()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        static unsigned long lastTime = 0;
        if (millis() - lastTime > 2000)
        {
            lastTime = millis();
            webSocketClient.sendTXT("{\"heartbeat\": true}");
        }
    }
}

bool render_alarm = true;

void checkAndSendAlarm()
{
    int pinAlarm = digitalRead(alarm_detected_pin);

    if (pinAlarm == HIGH && render_alarm == true)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            HTTPClient http;

            // Construye la URL con parámetros (ajusta según tu backend)
            String url = "http://192.168.1.108:5001/call_alarm";

            http.begin(url);

            int httpCode = http.GET();
            http.end();
            buttonsState.alarm = true;
            sendUpdatedState();
            render_alarm = false;
        }
        else
        {
            connectToWiFi();
        }
    }

    if (pinAlarm == LOW && render_alarm == false)
    {
        buttonsState.alarm = false;
        if (WiFi.status() == WL_CONNECTED)
        {
            sendUpdatedState();
        }

        render_alarm = true;
    }
}
void loop()
{
    webSocketClient.loop();

    ethernetControl();

    checkAndSendAlarm();

    checkEthernetDisconnect();

    checkDoor();

    closeDoor();

}
