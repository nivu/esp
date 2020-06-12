// Thermocouple
#include "max6675.h"
int thermoDO = 4; //D2
int thermoCS = 5; //D1
int thermoCLK = 0; //D3

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

//int vccPin = 14; //D5
//int gndPin = 2; //D4

// Arduino Pins used to control CD4067BE MUX
int A = 14; //D5
int B = 12; //D6
int C = 13; //D7
int D = 15; //d8

void setup() {
  
//  Arduino pins used to control max6675
//  pinMode(vccPin, OUTPUT); digitalWrite(vccPin, HIGH);
//  pinMode(gndPin, OUTPUT); digitalWrite(gndPin, LOW);

  pinMode(A, OUTPUT); digitalWrite(A, LOW);
  pinMode(B, OUTPUT); digitalWrite(B, LOW);
  pinMode(C, OUTPUT); digitalWrite(C, LOW);
  pinMode(D, OUTPUT); digitalWrite(D, LOW);
  
  Serial.begin(115200);

}
int out0;
int out1;

void loop() {
  digitalWrite(A, LOW);
  digitalWrite(B, LOW);
  digitalWrite(C, LOW);
  digitalWrite(D, LOW);
  delay(1000);
  out0 = thermocouple.readCelsius();
  Serial.print("Out 0 : ");Serial.println(out0);
  
  digitalWrite(A, HIGH);
  digitalWrite(B, LOW);
  digitalWrite(C, LOW);
  digitalWrite(D, LOW);
  delay(1000);
  out1 = thermocouple.readCelsius();
  Serial.print("Out 1 : ");Serial.println(out1);

  digitalWrite(A, LOW);
  digitalWrite(B, HIGH);
  digitalWrite(C, LOW);
  digitalWrite(D, LOW);
  delay(1000);
  out1 = thermocouple.readCelsius();
  Serial.print("Out 2 : ");Serial.println(out1);

  digitalWrite(A, HIGH);
  digitalWrite(B, HIGH);
  digitalWrite(C, LOW);
  digitalWrite(D, LOW);
  delay(1000);
  out1 = thermocouple.readCelsius();
  Serial.print("Out 3 : ");Serial.println(out1);
  Serial.println(" ");
}

