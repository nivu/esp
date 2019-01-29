/*******************************************
An Example program for GY-MCU90615
Code Written by Navaneeth Malingan (www.nivu.me)
www.github.com/navneetnivu07
Connections:
Sensor VCC -> Arduino 5V
Sensor GND -> Arduino GND
Sensor Rx  -> Arduino 11
Sensor Tx  -> Arduino 10

Serial port settings:
Set serial monitor baud rate to 57600

Working of my code:
The code utilizes Software Serial library to
communicate with GY-MCU90615. So, that normal
communication sould stay intact within Arduino 
& PC. First, begin serial communication with the sensor at 
115200 bauds(default). Then send instruction HEX commands
to enable continuous Communication (more info at 
http://www.ekt2.com/pdf/412_ARDUINO_INFRARED_THERMOMETER_MODULE.pdf)

Now, in loop section, We check if serial communication
with sensor is established. Then, we take 32 readings
and now check for the starting of the output data by checking
for 0x5A. If found, we increment by +4 & +5 and then 
retrieve the temperature data by the formula
temp = (4thByte | 5thByte)/100 

*******************************************/


HardwareSerial mySerial1(1);
#define RXD1 4
#define TXD1 2

unsigned char output[9];
unsigned char need[2];
unsigned char needd[2];

//interrupt
static volatile uint16_t intTriggerCount=0;
long debouncing_time = 1000; // in ms
volatile unsigned long last_micros;

void IRAM_ATTR isr(){  //IRAM_ATTR tells the complier, that this code Must always be in the 
// ESP32's IRAM, the limited 128k IRAM.  use it sparingly.

  if((long)(micros() - last_micros) >= debouncing_time * 1000) {
    intTriggerCount++;
    last_micros = micros();
  }
}

void setup() {
  pinMode(25,INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(25),isr,RISING);
  Serial.begin(115200);
  while(!Serial){
    ;  
  }
  mySerial1.begin(115200, SERIAL_8N1, RXD1, TXD1);
}

void loop() {

  static uint16_t old_Value = 0;

  if(old_Value != intTriggerCount){
      old_Value = intTriggerCount; // something to compare against
      Serial.println();
      Serial.printf("Interrupt pin 25, it is the %d's time!",intTriggerCount);
      Serial.println();
    
      mySerial1.write(0xA5);
      mySerial1.write(0x15);
      mySerial1.write(0xBA);
  }
  if(mySerial1.available()){
    Serial.print("Taking Readings");
    for(int counter=0;counter<=8;counter++){
      output[counter] = (unsigned char)mySerial1.read();
      Serial.print(".");
    }
    for(int obj=0;obj<=8;obj++){
      Serial.print(output[obj], HEX);
      Serial.print(" - ");
    }
      if(output[0]==0x5A && output[1]==0x5A && output[2]==0x45){
        need[0] = output[4];
        need[1] = output[5];
        needd[0] = output[6];
        needd[1] = output[7];
        float targetTempC = (float)(need[0] << 8 | need[1])/100;
        float ambientTempC = (float)(needd[0] << 8 | needd[1])/100;

        float targetTempF = (targetTempC * 1.8) + 32;
        float ambientTempF = (ambientTempC* 1.8) + 32;
        Serial.println();

        // In Celcius
        //Serial.print("Target Temperature is: ");Serial.print(targetTempC);Serial.println("C");
        //Serial.print("Ambient Temperature is: ");Serial.print(ambientTempC);Serial.println("C");

        // In Fahrenheit
        Serial.print("Target Temperature is: ");Serial.print(targetTempF);Serial.println("F");
        Serial.print("Ambient Temperature is: ");Serial.print(ambientTempF);Serial.println("F");
      }
  }
}
