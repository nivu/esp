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
StaticJsonBuffer<200> jsonBuffer;

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
  JsonObject& jsonIn = jsonBuffer.parseObject(receivedData);
  if (jsonIn.success()) {
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
      JsonObject& jsonOut = jsonBuffer.createObject();
      jsonOut["ssidPrim"] = ssidPrim;
      jsonOut["pwPrim"] = pwPrim;
      jsonOut["ssidSec"] = ssidSec;
      jsonOut["pwSec"] = pwSec;
      // Convert JSON object into a string
      jsonOut.printTo(wifiCredentials);

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
  } else {
    Serial.println("Received invalid JSON");
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
} 
