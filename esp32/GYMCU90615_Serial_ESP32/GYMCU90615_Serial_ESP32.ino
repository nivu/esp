/*******************************************
An Example program for GY-MCU90615
Code Written by Shivam Gautam (www.amritsar.tech)
www.github.com/shivamgautam1
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


HardwareSerial mySerial2(2);
#define RXD2 16
#define TXD2 17

unsigned char output[33];
unsigned char need[2];

void setup() {
  Serial.begin(9600);
  while(!Serial){
    ;  
  }
  
  mySerial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
}

void loop() {
  mySerial2.write(0xA5);
  mySerial2.write(0x15);
  mySerial2.write(0xBA);
  
  if(mySerial2.available()){
    Serial.print("Taking Readings");
    for(int counter=0;counter<=32;counter++){
      output[counter] = (unsigned char)mySerial2.read();
      Serial.print(".");
    }
    for(int obj=0;obj<=32;obj++){
      Serial.print(output[obj]);
      Serial.println(" ");
      if(output[obj]==0x5A){
        need[0] = output[obj+4];
        need[1] = output[obj+5];
        float temp = (float)(need[0] << 8 | need[1])/100;
        Serial.println();
        Serial.println("Temperature is: ");
        Serial.print(temp);
      }
      else{//No need to do anything}  
    }

    }
  }
      delay(10000);
}
