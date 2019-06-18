#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// https://arduinojson.org/v6/doc/installation/
 
const char* ssid = "TP-LINK_DE40"; //"KRISH_FTTH";
const char* password =  "password@1234"; // "KRISHtec@5747";
const char* mqttServer = "broker.hivemq.com"; //iot.eclipse.org
const int mqttPort = 1883;  
const char* mqttUser = "";  
const char* mqttPassword = "";

int nodeId = 1; // change to your respective node id
 
WiFiClient espClient;
PubSubClient client(espClient);
 
void setup() {
 
  Serial.begin(115200);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
 
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
 
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
 
    if (client.connect("ESP8266Client-kt" + nodeId, mqttUser, mqttPassword )) {
 
      Serial.println("connected");  
 
    } else {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
 
    }
  }
 
  //client.publish("kt-data", "Hello from ESP8266");
  client.subscribe("kt-control");
 
}
 
void callback(char* topic, byte* payload, unsigned int length) {
 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
 
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

  Serial.println("");
   
  if((char)payload[0] == '1') {
    Serial.println("LED ON");
  } else {
    Serial.println("LED OFF");
  }
 
  Serial.println();
  Serial.println("-----------------------");
 
}
 
void loop() {
  client.loop();

  int pin = 1;
  int value = analogRead(A0);
  char buffer[512];

  StaticJsonDocument<200> doc;
  doc["node"] = nodeId;
  doc["pin"] = pin;
  doc["value"] = value;
  serializeJson(doc, buffer);

  client.publish("kt-data/1", buffer);
  
  delay(5000);

}
