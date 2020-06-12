#define LED_PIN 2

void setup()
{
  Serial.begin(115200);
  delay(1000); // give me time to bring up serial monitor
  Serial.println("ESP32 Touch Test");
  pinMode(LED_PIN, OUTPUT);
  digitalWrite (LED_PIN, LOW);
}

void loop()
{
  digitalWrite (LED_PIN, HIGH);
  delay(1000);
  digitalWrite (LED_PIN, LOW);
  delay(1000);
}
