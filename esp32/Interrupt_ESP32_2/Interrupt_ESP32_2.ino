// Ref : https://github.com/espressif/arduino-esp32/issues/855

static volatile uint16_t intTriggerCount=0; // this variable will be changed in the ISR, and Read in main loop
// static: says this variable is only visible to function in this file, its value will persist, it is a global variable
// volatile: tells the compiler that this variables value must not be stored in a CPU register, it must exist
//   in memory at all times.  This means that every time the value of intTriggeredCount must be read or
//   changed, it has be read from memory, updated, stored back into RAM, that way, when the ISR 
//   is triggered, the current value is in RAM.  Otherwise, the compiler will find ways to increase efficiency
//   of access.  One of these methods is to store it in a CPU register, but if it does that,(keeps the current
//   value in a register, when the interrupt triggers, the Interrupt access the 'old' value stored in RAM, 
//   changes it, then returns to whatever part of the program it interrupted.  Because the foreground task,
//   (the one that was interrupted) has no idea the RAM value has changed, it uses the value it 'know' is 
//   correct (the one in the register).  

long debouncing_time = 1000; // in ms
volatile unsigned long last_micros;

void IRAM_ATTR isr(){  //IRAM_ATTR tells the complier, that this code Must always be in the 
// ESP32's IRAM, the limited 128k IRAM.  use it sparingly.

  if((long)(micros() - last_micros) >= debouncing_time * 1000) {
    intTriggerCount++;
    last_micros = micros();
  }

}

void setup(){
Serial.begin(9600);
pinMode(25,INPUT_PULLDOWN);
attachInterrupt(digitalPinToInterrupt(25),isr,RISING);
}

void loop(){
// magic happens her
static uint16_t old_Value=0; // this variable is only visible inside loop(),
// but it is persistent, It is only init'ed
// once, and each time through loop() it remembers its prior value

if(old_Value != intTriggerCount){
  old_Value = intTriggerCount; // something to compare against
  Serial.printf(" Someone grounded pin 25 again, it is the %d's time! Better call the Cops!",intTriggerCount);
  Serial.println(" ");
}
}
