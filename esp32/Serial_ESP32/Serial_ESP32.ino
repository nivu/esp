HardwareSerial mySerial2(2);
#define RXD2 16
#define TXD2 17

HardwareSerial mySerial0(0);
#define RXD1 3
#define TXD1 1

String inputString = ""; // a String to hold incoming data
boolean stringComplete = false; // whether the string is complete

void setup() {
  // Note the format for setting a serial port is as follows: Serial2.begin(baud-rate, protocol, RX pin, TX pin);
  Serial.begin(9600);
    while(!Serial){
    ;  
  }
  mySerial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  mySerial0.begin(9600, SERIAL_8N1, RXD1, TXD1);
}

void loop() { //Choose Serial1 or Serial2 as required
  while (mySerial0.available()) {
    char inChar = (char)mySerial0.read();
    inputString += inChar;
    if (inChar == '\r') {
      stringComplete = true;
      Serial.println(inputString);
      inputString = "";
    }
    //delay(1000);
  }
  while (mySerial2.available()) {
    char inChar = (char)mySerial2.read();
    inputString += inChar;
    if (inChar == '\r') {
      stringComplete = true;
      Serial.println(inputString);
      inputString = "";
    }
    //delay(1000);
  }
}
