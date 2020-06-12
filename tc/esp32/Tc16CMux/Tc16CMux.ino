#include "max6675.h"

// Thermocouple 1
int thermoDO1 = 18; //D18
int thermoCS1 = 19; //D19
int thermoCLK1 = 21; //D21

MAX6675 thermocouple1(thermoCLK1, thermoCS1, thermoDO1);

// Arduino Pins used to control 74HC4067 MUX
int A = 5; //D5 S0
int B = 4; //D4 S1
int C = 2; //D2 S2 
int D = 15; //D15 S3

float tc1_out;
float tc2_out;

void setup() {
  pinMode(A, OUTPUT); digitalWrite(A, HIGH);
  pinMode(B, OUTPUT); digitalWrite(B, HIGH);
  pinMode(C, OUTPUT); digitalWrite(C, HIGH);
  pinMode(D, OUTPUT); digitalWrite(D, HIGH);
  Serial.begin(115200);
}

void loop() {
  for(int i = 15; i > 7; i--) {
    String bin = String(i, BIN);
    Serial.println(bin); 
    (String(bin[0]) == "1") ? digitalWrite(D, HIGH) : digitalWrite(D, LOW);
    (String(bin[1]) == "1") ? digitalWrite(C, HIGH) : digitalWrite(C, LOW);
    (String(bin[2]) == "1") ? digitalWrite(B, HIGH) : digitalWrite(B, LOW);
    (String(bin[3]) == "1") ? digitalWrite(A, HIGH) : digitalWrite(A, LOW);
    delay(1000);
    tc1_out = thermocouple1.readCelsius();
    Serial.print("CH_" + String(i) + " : ");Serial.println(tc1_out);
    Serial.println(" ");
  }
}

