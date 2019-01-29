#include <WiFi.h>

HardwareSerial mySerial2(2);
#define RXD2 16
#define TXD2 17

HardwareSerial mySerial0(0);
#define RXD1 3
#define TXD1 1

String inputString = ""; // a String to hold incoming data
boolean stringComplete = false; // whether the string is complete

const char* ssid     = "KRISH_FTTH";
const char* password = "KRISHtec@5747";
const char* host = "livemonitoring.co.in";
//const char* host = "192.168.0.107";

int device_id = 1;

// Interrupt handler
// Ref : https://www.instructables.com/id/Arduino-Software-debouncing-in-interrupt-function/
// Ref : https://www.switchdoc.com/2018/04/esp32-tutorial-debouncing-a-button-press-using-interrupts/


static volatile uint16_t intTriggerCount=0;
long debouncing_time = 1000; // in ms
volatile unsigned long last_micros;

void IRAM_ATTR isr(){  //IRAM_ATTR tells the complier, that this code Must always be in the 
// ESP32's IRAM, the limited 128k IRAM.  use it sparingly.
  if((long)(micros() - last_micros) >= debouncing_time * 1000) {
    intTriggerCount++;
    last_micros = micros();
  }
}

void setup() {
  pinMode(25,INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(25),isr,RISING);

  Serial.begin(9600);
  mySerial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  mySerial0.begin(9600, SERIAL_8N1, RXD1, TXD1);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  delay(10);

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

void insertBpData(int sys, int dia, int pul) {
  // Appending values
  String params = "did=" + String(device_id) + "&s=" + String(sys) + "&d=" + String(dia) + "&p=" + String(pul);
  
  // We now create a URI for the request
  String bpUrl = "/bmm/insert.php?" + params;
  insertData(bpUrl);
}

void insertWeightData(float w) {
  // Appending values
  String params = "did=" + String(device_id) + "&w=" + String(w);
  
  // We now create a URI for the request
  String wUrl = "/bmm/insert_wh.php?" + params;
  insertData(wUrl);
}

void insertData(String url) {
  Serial.print("connecting to ");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
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

int tempRead = 0;
int weiRead = 0;

void loop() {
  static uint16_t old_Value=0;
  
  while (mySerial2.available() > 0) {
    String resp = mySerial2.readString();
    Serial.println(resp);
    // format 129, 107, 095
    String sys = resp.substring(0, 4);   
    String dia = resp.substring(5, 9); 
    String pul = resp.substring(10, 14);
    Serial.println("systole:" + sys + " diastole:" + dia + " pulse:" + pul);

    if(sys.toInt() > 60 && sys.toInt() < 200 && dia.toInt() > 30 && dia.toInt() < 110){
        insertBpData(sys.toInt(), dia.toInt(), pul.toInt());
    }
  }

    if(old_Value != intTriggerCount){
      old_Value = intTriggerCount; // something to compare against
      tempRead = 1;
      Serial.printf(" Someone grounded pin 25 again, it is the %d's time!",intTriggerCount);
      Serial.println(" ");
    }
    
   while (mySerial0.available() > 0 && tempRead == 1) {
      char inChar = (char)mySerial0.read();
      inputString += inChar;
      if (inChar == '\r') {
        weiRead+=1;
        stringComplete = true;
        Serial.println(inputString + " " + weiRead);
        if(weiRead == 2){
          insertWeightData(inputString.toFloat());
          weiRead = 0;
          tempRead = 0;
        }
        inputString = "";
      }
    }
}





