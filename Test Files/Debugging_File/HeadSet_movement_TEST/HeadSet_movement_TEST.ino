// ===== SIMPLE DIRECTION TEST (no raw numbers, just plain messages) =====
// Prints "Looking Up", "Looking Down", "Looking Left", "Looking Right", "Tilted Left", "Tilted Right", "Centered"
// Button (D33) resets position and prints "POSITION RESET - Center"

#include <Wire.h>

#define BLUE_LED 25
#define GREEN_LED 26
#define MPU_ADDR 0x68
#define CALIBRATE_BTN 33

int16_t ax, ay, az, gx, gy, gz;
float pitch = 0, roll = 0, yaw = 0;

unsigned long lastTime = 0;
unsigned long lastPrint = 0;
unsigned long lastButtonPress = 0;
int lastButtonState = HIGH;

String lastDirection = "";

const float THRESHOLD = 8.0;   // degrees - how far you must move to count as a direction
const int DEBOUNCE_DELAY = 300;

void readMPU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true);

  ax = Wire.read() << 8 | Wire.read();
  ay = Wire.read() << 8 | Wire.read();
  az = Wire.read() << 8 | Wire.read();
  Wire.read(); Wire.read();
  gx = Wire.read() << 8 | Wire.read();
  gy = Wire.read() << 8 | Wire.read();
  gz = Wire.read() << 8 | Wire.read();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== DIRECTION TEST START ===");
  Serial.println("Move the headset up/down/left/right and press the button to reset.");

  pinMode(BLUE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(CALIBRATE_BTN, INPUT_PULLUP);
  digitalWrite(GREEN_LED, HIGH);

  Wire.begin(21, 22);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  delay(100);

  lastTime = millis();
}

void loop() {
  readMPU();

  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0;
  lastTime = now;

  float accPitch = atan2(ay, az) * 180.0 / PI;
  float accRoll  = atan2(ax, az) * 180.0 / PI;
  float gyroPitchRate = gx / 131.0;
  float gyroRollRate  = gy / 131.0;
  float gyroYawRate   = gz / 131.0;

  pitch = 0.98 * (pitch + gyroPitchRate * dt) + 0.02 * accPitch;
  roll  = 0.98 * (roll + gyroRollRate * dt) + 0.02 * accRoll;
  yaw   = yaw + gyroYawRate * dt;

  if (yaw > 180) yaw -= 360;
  if (yaw < -180) yaw += 360;

  // ---- Button: reset position ----
  int currentButtonState = digitalRead(CALIBRATE_BTN);
  if (currentButtonState == LOW && lastButtonState == HIGH && (now - lastButtonPress > DEBOUNCE_DELAY)) {
    yaw = 0;
    pitch = 0;
    roll = 0;
    lastButtonPress = now;

    Serial.println(">>> POSITION RESET - Center <<<");
    lastDirection = "";

    for (int i = 0; i < 3; i++) {
      digitalWrite(BLUE_LED, HIGH);
      digitalWrite(GREEN_LED, HIGH);
      delay(150);
      digitalWrite(BLUE_LED, LOW);
      digitalWrite(GREEN_LED, LOW);
      delay(150);
    }
    digitalWrite(GREEN_LED, HIGH);
  }
  lastButtonState = currentButtonState;

  // ---- Determine direction, only print when it CHANGES ----
  if (now - lastPrint > 200) {
    String direction = "Centered";

    if (pitch > THRESHOLD) direction = "Looking Down";
    else if (pitch < -THRESHOLD) direction = "Looking Up";
    else if (yaw > THRESHOLD) direction = "Looking Right";
    else if (yaw < -THRESHOLD) direction = "Looking Left";
    else if (roll > THRESHOLD) direction = "Tilted Right";
    else if (roll < -THRESHOLD) direction = "Tilted Left";

    if (direction != lastDirection) {
      Serial.println("Position changed: " + direction);
      digitalWrite(BLUE_LED, direction == "Centered" ? LOW : HIGH);
      lastDirection = direction;
    }

    lastPrint = now;
  }
}