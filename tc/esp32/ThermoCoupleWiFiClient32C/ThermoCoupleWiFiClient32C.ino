// Board : DIOT ESP32 DEVKIT V1

// WiFi
#include <WiFi.h>
//const char* ssid     = "KRISH_FTTH";
//const char* password = "KRISHtec@5747";
const char* ssid     = "SRECCIET";
const char* password = "Ajith@007";
const char* host = "livemonitoring.co.in";

// SPI Interface Data, Clk, ChipSelect
#include "max6675.h"
// Thermocouple 1
int thermoDO1 = 18; //D18
int thermoCS1 = 19; //D19
int thermoCLK1 = 21; //D21

// Thermocouple 2
int thermoDO2 = 34; //D34
int thermoCS2 = 33; //D35 ->33
int thermoCLK2 = 32; //D32

MAX6675 thermocouple1(thermoCLK1, thermoCS1, thermoDO1);
MAX6675 thermocouple2(thermoCLK2, thermoCS2, thermoDO2);

// Arduino Pins used to control 74HC4067 MUX
int A = 5; //D5 S0
int B = 4; //D4 S1
int C = 2; //D2 S2 
int D = 15; //D15 S3

int mux1_pin_map[] = {16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1};
int mux2_pin_map[] = {17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32};


// Analog Input: ADC_1_0 pin ==> GPIO36 (VP).
// Ref : https://www.instructables.com/id/IOT-Made-Simple-Playing-With-the-ESP32-on-Arduino-/
#define ANALOG_PIN_0 36
int analog_value = 0;

void setup() {

  pinMode(A, OUTPUT); digitalWrite(A, HIGH);
  pinMode(B, OUTPUT); digitalWrite(B, HIGH);
  pinMode(C, OUTPUT); digitalWrite(C, HIGH);
  pinMode(D, OUTPUT); digitalWrite(D, HIGH);
  
  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
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

float tcCalib(float val) {
  if(val <= 40) {
    return val - 7;
  } else if(val >= 41 &&  val <= 70) {
    return val - 6;
  } else if(val >= 71) {
    return val - 7;
  } else if(val == 0) {
    return 0;
  } else {
    return 0;
  }
}

void loop() {
  String params;
  float tc1_out;
  float tc2_out;
  for(int i = 15; i >= 0; i--) {
    String bin = String(i, BIN);
    Serial.println(bin);
    switch(bin.length()){
      case 1:
        bin = "000" + bin;
        break;
      case 2:
        bin = "00" + bin;
        break;
      case 3:
        bin = "0" + bin;
        break;
      default:
        break;
    }
    Serial.println(bin);
    (String(bin[0]) == "1") ? digitalWrite(D, HIGH) : digitalWrite(D, LOW);
    (String(bin[1]) == "1") ? digitalWrite(C, HIGH) : digitalWrite(C, LOW);
    (String(bin[2]) == "1") ? digitalWrite(B, HIGH) : digitalWrite(B, LOW);
    (String(bin[3]) == "1") ? digitalWrite(A, HIGH) : digitalWrite(A, LOW);
    delay(200);
    tc1_out = thermocouple1.readCelsius();
    if(tc1_out == tc1_out){
       tc1_out = tcCalib(tc1_out);
    } else {
       tc1_out = 0;
    }
    tc2_out = thermocouple2.readCelsius();
    if(tc2_out == tc2_out){
      tc2_out = tcCalib(tc2_out);
    } else {
      tc2_out = 0;
    }
    Serial.println(mux1_pin_map[i]); 
    params = params + "ch" + mux1_pin_map[i] + '=' + tc1_out + '&';
    params = params + "ch" + mux2_pin_map[i] + '=' + tc2_out + '&';
    Serial.print("CH_" + String(i) + " : ");Serial.println(tc1_out);
    Serial.println(" ");
  }

  // Appending analog value
  params = params + "a0=" +   analog_value;

  // ch16=10&ch18=12&a0=35

  Serial.print("connecting to ");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  Serial.println(params);

  // We now create a URI for the request
  String url = "/ciet/scripts/insert.php?" + params;
  
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
