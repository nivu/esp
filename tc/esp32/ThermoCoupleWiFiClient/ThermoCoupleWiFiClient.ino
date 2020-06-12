#include <ESP8266WiFi.h>
#include "max6675.h"

const char* ssid     = "KRISH_FTTH";
const char* password = "KRISHtec@5747";
const char* host = "192.168.1.104";

// Thermocouple 1
int thermoDO1 = 18; //D18
int thermoCS1 = 19; //D19
int thermoCLK1 = 21; //D21

// Thermocouple 2
int thermoDO2 = 34; //D34
int thermoCS2 = 35; //D35
int thermoCLK3 = 32; //D32

MAX6675 thermocouple1(thermoCLK1, thermoCS1, thermoDO1);
MAX6675 thermocouple2(thermoCLK2, thermoCS2, thermoDO2);

// Arduino Pins used to control 74HC4067 MUX
int A = 15; //D15
int B = 2; //D2
int C = 4; //D4
int D = 5; //D5

void setup() {

  pinMode(A, OUTPUT); digitalWrite(A, LOW);
  pinMode(B, OUTPUT); digitalWrite(B, LOW);
  pinMode(C, OUTPUT); digitalWrite(C, LOW);
  pinMode(D, OUTPUT); digitalWrite(D, LOW);
  
  Serial.begin(115200);
  delay(10);
/*
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
 
  /*WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());*/
}

void loop() {
  delay(5000);
  int value1 = thermocouple1.readCelsius();
  Serial.print("connecting to ");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
  // We now create a URI for the request
  // /insert.php?a=1&d=2
  String url = "/insert.php";
  url += "?a=";
  url += value;
  url += "&d=";
  url += 2;
  
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  
  Serial.println();
  Serial.println("closing connection");
}

