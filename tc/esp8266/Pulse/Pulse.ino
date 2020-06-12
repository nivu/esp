// Arduino Pins used to control CD4067BE MUX
int A = 14; //D5
int B = 12; //D6
int C = 13; //D7
int D = 15; //d8

void setup() {
  
  pinMode(A, OUTPUT); digitalWrite(A, LOW);
  pinMode(B, OUTPUT); digitalWrite(B, LOW);
  pinMode(C, OUTPUT); digitalWrite(C, LOW);
  
  pinMode(D, OUTPUT); digitalWrite(D, LOW);
  
  Serial.begin(115200);

}
int out0;
int out1;

void loop() {
//  digitalWrite(A, LOW);
//  digitalWrite(B, LOW);
//  digitalWrite(C, LOW);
  
  digitalWrite(D, HIGH);
  delay(100);
  digitalWrite(D, LOW);
  delay(100);
/*
  digitalWrite(A, HIGH);
  digitalWrite(B, LOW);
  digitalWrite(C, LOW);
  delay(10);
  digitalWrite(D, HIGH);
  delay(10);
  digitalWrite(D, LOW);
  delay(10);
  */
}

