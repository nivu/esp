#include <WiFi.h>

// Weight Machine
HardwareSerial mySerial0(0);
#define RXD0 3
#define TXD0 1

// Temperature Sensor
HardwareSerial mySerial1(1);
#define RXD1 4
#define TXD1 2

// Bp Sensor
HardwareSerial mySerial2(2);
#define RXD2 16
#define TXD2 17


// To Hold Temp SEnsor Data
unsigned char output[9];
unsigned char need[2];
unsigned char needd[2];

// To hold weight machine data
String inputString = ""; // a String to hold incoming data
boolean stringComplete = false; // whether the string is complete

// network details
const char* ssid     = "KRISH_FTTH";
const char* password = "KRISHtec@5747";
const char* host = "livemonitoring.co.in";
//const char* host = "192.168.0.107";

// device id
int device_id = 1;

// Interrupt handler
// Ref : https://www.instructables.com/id/Arduino-Software-debouncing-in-interrupt-function/
// Ref : https://www.switchdoc.com/2018/04/esp32-tutorial-debouncing-a-button-press-using-interrupts/

// Interrupt 1
static volatile uint16_t intTriggerCount=0;
long debouncing_time = 1000; // in ms
volatile unsigned long last_micros;

void IRAM_ATTR isr(){  
  if((long)(micros() - last_micros) >= debouncing_time * 1000) {
    intTriggerCount++;
    last_micros = micros();
  }
}

// Interrupt 2
static volatile uint16_t intTriggerCount2=0;
long debouncing_time2 = 1000; // in ms
volatile unsigned long last_micros2;

void IRAM_ATTR isr2(){  
  if((long)(micros() - last_micros2) >= debouncing_time2 * 1000) {
    intTriggerCount2++;
    last_micros2 = micros();
  }
}

void setup() {
  pinMode(25,INPUT_PULLDOWN);
  pinMode(26,INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(25),isr,RISING);
  attachInterrupt(digitalPinToInterrupt(26),isr2,RISING);

  Serial.begin(9600);
  mySerial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  mySerial1.begin(115200, SERIAL_8N1, RXD1, TXD1);
  mySerial0.begin(9600, SERIAL_8N1, RXD0, TXD0);

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  delay(10);

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
  String params = "did=" + String(device_id) + "&s=" + String(sys) + "&d=" + String(dia) + "&p=" + String(pul);
  String bpUrl = "/bmm/insert.php?" + params;
  insertData(bpUrl);
}

void insertWeightData(float w) {
  String params = "did=" + String(device_id) + "&w=" + String(w);
  String wUrl = "/bmm/insert_wh.php?" + params;
  insertData(wUrl);
}

void insertTempData(float t) {
  String params = "did=" + String(device_id) + "&t=" + String(t);
  String tUrl = "/bmm/insert_temp.php?" + params;
  insertData(tUrl);
}

void insertData(String url) {
  Serial.print("connecting to ");
  Serial.println(host);
  
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
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
  static uint16_t old_Value = 0;
  static uint16_t old_Value2 = 0;

  if(old_Value2 != intTriggerCount2){
      old_Value2 = intTriggerCount2; // something to compare against
      Serial.println();
      Serial.printf("Interrupt pin 26, it is the %d's time!",intTriggerCount2);
      Serial.println();
    
      mySerial1.write(0xA5);
      mySerial1.write(0x15);
      mySerial1.write(0xBA);
  }

  if(mySerial1.available()){
    Serial.print("Taking Readings");
    for(int counter=0;counter<=8;counter++){
      output[counter] = (unsigned char)mySerial1.read();
      Serial.print(".");
    }
      if(output[0]==0x5A && output[1]==0x5A && output[2]==0x45){
        need[0] = output[4];
        need[1] = output[5];
        needd[0] = output[6];
        needd[1] = output[7];
        float targetTempC = (float)(need[0] << 8 | need[1])/100;
        float ambientTempC = (float)(needd[0] << 8 | needd[1])/100;

        float targetTempF = (targetTempC * 1.8) + 32;
        float ambientTempF = (ambientTempC* 1.8) + 32;
        Serial.println();
        
        // In Fahrenheit
        Serial.print("Target Temperature is: ");Serial.print(targetTempF);Serial.println("F");
        Serial.print("Ambient Temperature is: ");Serial.print(ambientTempF);Serial.println("F");
        insertTempData(targetTempF);
      }
  }
  
  while (mySerial2.available() > 0) {
    String resp = mySerial2.readString();
    Serial.println(resp);
    // format 129, 107, 095
    String sys = resp.substring(0, 4);   
    String dia = resp.substring(5, 9); 
    String pul = resp.substring(10, 14);
    Serial.println("systole:" + sys + " diastole:" + dia + " pulse:" + pul);

    if(sys.toInt() > 60 && sys.toInt() < 200 && dia.toInt() > 30 && dia.toInt() < 150){
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





