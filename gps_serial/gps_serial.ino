#include <SoftwareSerial.h>

// software serial #2: RX = digital pin 8, TX = digital pin 9
// on the Mega, use other pins instead, since 8 and 9 don't work on the Mega
SoftwareSerial portTwo(8, 9);

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  portTwo.begin(9600);
}

void loop() {

  while (portTwo.available() > 0) {
    char inByte = portTwo.read();
    Serial.write(inByte);
  }
}
