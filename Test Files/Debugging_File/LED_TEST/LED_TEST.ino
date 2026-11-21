// ===== LED TEST =====
// Tests Blue (GPIO 25) and Green (GPIO 26) LEDs independently

#define BLUE_LED 25
#define GREEN_LED 26

void setup() {
  Serial.begin(115200);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  Serial.println("=== LED TEST START ===");
}

void loop() {
  Serial.println("Blue ON, Green OFF");
  digitalWrite(BLUE_LED, HIGH);
  digitalWrite(GREEN_LED, LOW);
  delay(1000);

  Serial.println("Blue OFF, Green ON");
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(GREEN_LED, HIGH);
  delay(1000);

  Serial.println("Both ON");
  digitalWrite(BLUE_LED, HIGH);
  digitalWrite(GREEN_LED, HIGH);
  delay(1000);

  Serial.println("Both OFF");
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
  delay(1000);
}