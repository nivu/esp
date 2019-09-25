// Default Arduino includes
#include <Arduino.h>
#include <WiFi.h>
#include <nvs.h>
#include <nvs_flash.h>

// Includes for JSON object handling
// Requires ArduinoJson library
// https://arduinojson.org
// https://github.com/bblanchon/ArduinoJson
#include <ArduinoJson.h>

/* 

BLE tutorial: https://desire.giesecke.tk/index.php/2018/04/06/esp32-wifi-setup-over-ble/

Bluetooth tutorial: https://desire.giesecke.tk/index.php/2018/04/10/esp32-wifi-setup-over-bluetooth-serial/

Companion Android app: https://play.google.com/store/apps/details?id=tk.giesecke.esp32wifible

*/
// Includes for Bluetooth Serial
#include "BluetoothSerial.h"
#include <Preferences.h>

/** Build time */
const char compileDate[] = __DATE__ " " __TIME__;

/** Unique device name */
char apName[] = "ESP32-xxxxxxxxxxxx";
/** Selected network 
    true = use primary network
    false = use secondary network
*/
bool usePrimAP = true;
/** Flag if stored AP credentials are available */
bool hasCredentials = false;
/** Connection status */
volatile bool isConnected = false;
/** Connection change status */
bool connStatusChanged = false;

/**
 * Create unique device name from MAC address
 **/
void createName() {
  uint8_t baseMac[6];
  // Get MAC address for WiFi station
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  // Write unique name into apName
  sprintf(apName, "ESP32-%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
}

/** SSIDs of local WiFi networks */
String ssidPrim;
String ssidSec;
/** Password for local WiFi network */
String pwPrim;
String pwSec;

// SerialBT class
BluetoothSerial SerialBT;

/** Buffer for JSON string */
// MAx size is 51 bytes for frame: 
// {"ssidPrim":"","pwPrim":"","ssidSec":"","pwSec":""}
// + 4 x 32 bytes for 2 SSID's and 2 passwords
StaticJsonDocument<200> jsonBuffer;

/**
 * initBTSerial
 * Initialize Bluetooth Serial
 * Start BLE server and service advertising
 * @return <code>bool</code>
 *      true if success
 *      false if error occured
 */
bool initBTSerial() {
    if (!SerialBT.begin(apName)) {
      Serial.println("Failed to start BTSerial");
      return false;
    }
    Serial.println("BTSerial active. Device name: " + String(apName));
    return true;
}

/**
 * readBTSerial
 * read all data from BTSerial receive buffer
 * parse data for valid WiFi credentials
 */
void readBTSerial() {
  uint64_t startTimeOut = millis();
  String receivedData;
  int msgSize = 0;
  // Read RX buffer into String
  while (SerialBT.available() != 0) {
    receivedData += (char)SerialBT.read();
    msgSize++;
    // Check for timeout condition
    if ((millis()-startTimeOut) >= 5000) break;
  }
  SerialBT.flush();
  Serial.println("Received message " + receivedData + " over Bluetooth");

  // decode the message 
  int keyIndex = 0;
  for (int index = 0; index < receivedData.length(); index ++) {
    receivedData[index] = (char) receivedData[index] ^ (char) apName[keyIndex];
    keyIndex++;
    if (keyIndex >= strlen(apName)) keyIndex = 0;
  }

  Serial.println("Received message " + receivedData + " over Bluetooth");

  /** Json object for incoming data */
 // JsonObject& jsonIn = jsonBuffer.parseObject(receivedData);

  StaticJsonDocument<256> jsonIn;
  auto error = deserializeJson(jsonIn, receivedData);

  if (error){
    Serial.println("Received invalid JSON");
  } else {
    if (jsonIn.containsKey("ssidPrim") &&
        jsonIn.containsKey("pwPrim") && 
        jsonIn.containsKey("ssidSec") &&
        jsonIn.containsKey("pwSec")) {
      ssidPrim = jsonIn["ssidPrim"].as<String>();
      pwPrim = jsonIn["pwPrim"].as<String>();
      ssidSec = jsonIn["ssidSec"].as<String>();
      pwSec = jsonIn["pwSec"].as<String>();

      Preferences preferences;
      preferences.begin("WiFiCred", false);
      preferences.putString("ssidPrim", ssidPrim);
      preferences.putString("ssidSec", ssidSec);
      preferences.putString("pwPrim", pwPrim);
      preferences.putString("pwSec", pwSec);
      preferences.putBool("valid", true);
      preferences.end();

      Serial.println("Received over bluetooth:");
      Serial.println("primary SSID: "+ssidPrim+" password: "+pwPrim);
      Serial.println("secondary SSID: "+ssidSec+" password: "+pwSec);
      connStatusChanged = true;
      hasCredentials = true;
    } else if (jsonIn.containsKey("erase")) { // {"erase":"true"}
      Serial.println("Received erase command");
      Preferences preferences;
      preferences.begin("WiFiCred", false);
      preferences.clear();
      preferences.end();
      connStatusChanged = true;
      hasCredentials = false;
      ssidPrim = "";
      pwPrim = "";
      ssidSec = "";
      pwSec = "";

      int err;
      err=nvs_flash_init();
      Serial.println("nvs_flash_init: " + err);
      err=nvs_flash_erase();
      Serial.println("nvs_flash_erase: " + err);
    } else if (jsonIn.containsKey("read")) { // {"read":"true"}
      Serial.println("BTSerial read request");
      String wifiCredentials;
      jsonBuffer.clear();

      /** Json object for outgoing data */
      DynamicJsonDocument jsonOut(200);
      jsonOut["ssidPrim"] = ssidPrim;
      jsonOut["pwPrim"] = pwPrim;
      jsonOut["ssidSec"] = ssidSec;
      jsonOut["pwSec"] = pwSec;
      // Convert JSON object into a string
      serializeJson(jsonOut, wifiCredentials);

      // encode the data
      int keyIndex = 0;
      Serial.println("Stored settings: " + wifiCredentials);
      for (int index = 0; index < wifiCredentials.length(); index ++) {
        wifiCredentials[index] = (char) wifiCredentials[index] ^ (char) apName[keyIndex];
        keyIndex++;
        if (keyIndex >= strlen(apName)) keyIndex = 0;
      }
      Serial.println("Stored encrypted: " + wifiCredentials);

      delay(2000);
      SerialBT.print(wifiCredentials);
      SerialBT.flush();
    } else if (jsonIn.containsKey("reset")) {
      WiFi.disconnect();
      esp_restart();
    }
  }
  jsonBuffer.clear();
}

/** Callback for receiving IP address from AP */
void gotIP(system_event_id_t event) {
  isConnected = true;
  connStatusChanged = true;
}

/** Callback for connection loss */
void lostCon(system_event_id_t event) {
  isConnected = false;
  connStatusChanged = true;
}

/** Callback for connection loss */
void gotCon(system_event_id_t event) {
  Serial.println("Connection established, waiting for IP");
}

/** Callback for Station mode start */
void staStart(system_event_id_t event) {
  Serial.println("Station mode start");
}

/** Callback for Station mode stop */
void staStop(system_event_id_t event) {
  Serial.println("Station mode stop");
}

/**
   scanWiFi
   Scans for available networks 
   and decides if a switch between
   allowed networks makes sense

   @return <code>bool</code>
          True if at least one allowed network was found
*/
bool scanWiFi() {
  /** RSSI for primary network */
  int8_t rssiPrim;
  /** RSSI for secondary network */
  int8_t rssiSec;
  /** Result of this function */
  bool result = false;

  Serial.println("Start scanning for networks");

  WiFi.disconnect(true);
  WiFi.enableSTA(true);
  WiFi.mode(WIFI_STA);

  // Scan for AP
  int apNum = WiFi.scanNetworks(false,true,false,1000);
  if (apNum == 0) {
    Serial.println("Found no networks?????");
    return false;
  }
  
  byte foundAP = 0;
  bool foundPrim = false;

  for (int index=0; index<apNum; index++) {
    String ssid = WiFi.SSID(index);
    Serial.println("Found AP: " + ssid + " RSSI: " + WiFi.RSSI(index));
    if (!strcmp((const char*) &ssid[0], (const char*) &ssidPrim[0])) {
      Serial.println("Found primary AP");
      foundAP++;
      foundPrim = true;
      rssiPrim = WiFi.RSSI(index);
    }
    if (!strcmp((const char*) &ssid[0], (const char*) &ssidSec[0])) {
      Serial.println("Found secondary AP");
      foundAP++;
      rssiSec = WiFi.RSSI(index);
    }
  }

  switch (foundAP) {
    case 0:
      result = false;
      break;
    case 1:
      if (foundPrim) {
        usePrimAP = true;
      } else {
        usePrimAP = false;
      }
      result = true;
      break;
    default:
      Serial.printf("RSSI Prim: %d Sec: %d\n", rssiPrim, rssiSec);
      if (rssiPrim > rssiSec) {
        usePrimAP = true; // RSSI of primary network is better
      } else {
        usePrimAP = false; // RSSI of secondary network is better
      }
      result = true;
      break;
  }
  return result;
}

/**
 * Start connection to AP
 */
void connectWiFi() {
  // Setup callback function for successful connection
  WiFi.onEvent(gotIP, SYSTEM_EVENT_STA_GOT_IP);
  // Setup callback function for lost connection
  WiFi.onEvent(lostCon, SYSTEM_EVENT_STA_DISCONNECTED);
  // Setup callback function for connection established
  WiFi.onEvent(gotCon, SYSTEM_EVENT_STA_CONNECTED);
  // Setup callback function for connection established
  WiFi.onEvent(staStart, SYSTEM_EVENT_STA_START);
  // Setup callback function for connection established
  WiFi.onEvent(staStop, SYSTEM_EVENT_STA_STOP);

  WiFi.disconnect(true);
  WiFi.enableSTA(true);
  WiFi.mode(WIFI_STA);

  Serial.println();
  Serial.print("Start connection to ");
  if (usePrimAP) {
    Serial.println(ssidPrim);
    WiFi.begin(ssidPrim.c_str(), pwPrim.c_str());
  } else {
    Serial.println(ssidSec);
    WiFi.begin(ssidSec.c_str(), pwSec.c_str());
  }
}

/*********************** Thermocouple Code Starts ************************/
const char* host = "livemonitoring.co.in";

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
  // Create unique device name
  createName();

  // Initialize Serial port
  Serial.begin(115200);
  // Send some device info
  Serial.print("Build: ");
  Serial.println(compileDate);

  Preferences preferences;
  preferences.begin("WiFiCred", false);
  bool hasPref = preferences.getBool("valid", false);
  if (hasPref) {
    ssidPrim = preferences.getString("ssidPrim","");
    ssidSec = preferences.getString("ssidSec","");
    pwPrim = preferences.getString("pwPrim","");
    pwSec = preferences.getString("pwSec","");

    if (ssidPrim.equals("") 
        || pwPrim.equals("")
        || ssidSec.equals("")
        || pwPrim.equals("")) {
      Serial.println("Found preferences but credentials are invalid");
    } else {
      Serial.println("Read from preferences:");
      Serial.println("primary SSID: "+ssidPrim+" password: "+pwPrim);
      Serial.println("secondary SSID: "+ssidSec+" password: "+pwSec);
      hasCredentials = true;
    }
  } else {
    Serial.println("Could not find preferences, need send data over BLE");
  }
  preferences.end();

  // Start BTSerial
  initBTSerial();

  if (hasCredentials) {
    // Check for available AP's
    if (!scanWiFi) {
      Serial.println("Could not find any AP");
    } else {
      // If AP was found, start connection
      connectWiFi();
    }
  }

  pinMode(A, OUTPUT); digitalWrite(A, HIGH);
  pinMode(B, OUTPUT); digitalWrite(B, HIGH);
  pinMode(C, OUTPUT); digitalWrite(C, HIGH);
  pinMode(D, OUTPUT); digitalWrite(D, HIGH);
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

void readAndPost(){

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

  analog_value = analogRead(ANALOG_PIN_0);
  Serial.print("analog_value ");
  Serial.println(analog_value);
  // Appending analog value
  params = params + "a0=" +   analog_value;

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

void loop() {
  if (connStatusChanged) {
    if (isConnected) {
      Serial.print("Connected to AP: ");
      Serial.print(WiFi.SSID());
      Serial.print(" with IP: ");
      Serial.print(WiFi.localIP());
      Serial.print(" RSSI: ");
      Serial.println(WiFi.RSSI());
    } else {
      if (hasCredentials) {
        Serial.println("Lost WiFi connection");
        // Received WiFi credentials
        if (!scanWiFi) { // Check for available AP's
          Serial.println("Could not find any AP");
        } else { // If AP was found, start connection
          connectWiFi();
        }
      } 
    }
    connStatusChanged = false;
  }

  // Check if Data over SerialBT has arrived
  if (SerialBT.available() != 0) {
    // Get and parse received data
    readBTSerial();
  }

  if(isConnected) {
    readAndPost();
  }

  
} 
