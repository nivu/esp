  
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>

SoftwareSerial swSer(14, 12, false, 256);

const char* ssid     = "KRISH_FTTH";
const char* password = "KRISHtec@5747";
const char* host = "livemonitoring.co.in";
//const char* host = "192.168.0.107";

int device_id = 1;

void setup() {
  Serial.begin(9600);
  swSer.begin(9600);

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

void insertData(int sys, int dia, int pul) {
  // Appending values
  String params = "did=" + String(device_id) + "&s=" + String(sys) 
  + "&d=" + String(dia) + "&p=" + String(pul);

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
  String url = "/bmm/insert.php?" + params;
  
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
  while (swSer.available() > 0) {
    String resp = swSer.readString();
    Serial.println(resp);
    // format 129, 107, 095
    String sys = resp.substring(0, 4);   
    String dia = resp.substring(5, 9); 
    String pul = resp.substring(10, 14);
    Serial.println("systole:" + sys + " diastole:" + dia + " pulse:" + pul); 
    insertData(sys.toInt(), dia.toInt(), pul.toInt());
  }

}





