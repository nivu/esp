// Thermocouple
#include "max6675.h"
int thermoDO = 4; //D2
int thermoCS = 5; //D1
int thermoCLK = 0; //D3

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

void setup() {
  
//  Arduino pins used to control max6675
//  pinMode(vccPin, OUTPUT); digitalWrite(vccPin, HIGH);
//  pinMode(gndPin, OUTPUT); digitalWrite(gndPin, LOW);
  
  Serial.begin(115200);

}
float out0;

void loop() {
  out0 = thermocouple.readCelsius();
  Serial.print("Out 0 : ");Serial.println(out0 + 2);
  delay(1000);

}

