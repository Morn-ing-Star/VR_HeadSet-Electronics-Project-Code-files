// ===== VR HEADSET MODULE - FINAL VERSION (with hysteresis, no flicker + axis remap) =====
// Green LED: blinks during Pitch (up/down) OR Yaw/Roll (left/right/tilt) movement, solid ON when centered
// Blue LED: OFF during Pitch movement, ON solid during Yaw/Roll movement, ON solid when centered
// Both LEDs flash together 3x = calibration button pressed
// Sends Yaw,Pitch,Roll continuously over Bluetooth as "VR_Headset"

#include <Wire.h>
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

#define BLUE_LED 25
#define GREEN_LED 26
#define MPU_ADDR 0x68
#define CALIBRATE_BTN 33

// ---- Flip these to -1 if a direction feels backward once mounted ----
int INVERT_PITCH = 1;
int INVERT_ROLL = 1;
int INVERT_YAW = 1;

// ---- NEW: axis swap test - set true if up/down feels like left/right ----
bool SWAP_PITCH_YAW = false;
// -----------------------------------------------------------------

int16_t ax, ay, az, gx, gy, gz;
float pitch = 0, roll = 0, yaw = 0;
float pitchOffset = 0, rollOffset = 0, yawOffset = 0;

unsigned long lastTime = 0;
unsigned long lastSend = 0;
unsigned long lastGreenBlink = 0;
unsigned long lastButtonPress = 0;
int lastButtonState = HIGH;
bool greenState = false;

bool currentlyPitchMoving = false;
bool currentlySideMoving = false;

const float ENTER_THRESHOLD = 6.0;  // degrees - must exceed this to START counting as "moving"
const float EXIT_THRESHOLD = 3.0;   // degrees - must drop below this to go back to "centered"
const int SEND_INTERVAL = 20;       // ms
const int GREEN_BLINK_SPEED = 150;  // ms
const int DEBOUNCE_DELAY = 300;     // ms

void readMPU() {

  Serial.print("RAW ax:");
  Serial.print(ax);
  Serial.print(" ay:");
  Serial.print(ay);
  Serial.print(" az:");
  Serial.println(az);

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true);

  ax = Wire.read() << 8 | Wire.read();
  ay = Wire.read() << 8 | Wire.read();
  az = Wire.read() << 8 | Wire.read();
  Wire.read();
  Wire.read();
  gx = Wire.read() << 8 | Wire.read();
  gy = Wire.read() << 8 | Wire.read();
  gz = Wire.read() << 8 | Wire.read();
}

void flashBothLEDs(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BLUE_LED, HIGH);
    digitalWrite(GREEN_LED, HIGH);
    delay(150);
    digitalWrite(BLUE_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    delay(150);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== VR HEADSET MODULE START ===");

  pinMode(BLUE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(CALIBRATE_BTN, INPUT_PULLUP);
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(GREEN_LED, LOW);

  if (SerialBT.begin("VR_Headset")) {
    Serial.println("Bluetooth started: VR_Headset");
  } else {
    Serial.println("Bluetooth FAILED to start.");
  }

  Wire.begin(21, 22);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  delay(100);
  Serial.println("MPU sensor initialized.");

  flashBothLEDs(2);
  digitalWrite(BLUE_LED, HIGH);
  digitalWrite(GREEN_LED, HIGH);
  Serial.println("System ready. Streaming data now...");

  lastTime = millis();
}

void loop() {
  readMPU();

  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0;
  lastTime = now;

  float accPitch = atan2(ay, az) * 180.0 / PI;
  float accRoll = atan2(ax, az) * 180.0 / PI;
  float gyroPitchRate = gx / 131.0;
  float gyroRollRate = gy / 131.0;
  float gyroYawRate = gz / 131.0;

  pitch = 0.98 * (pitch + gyroPitchRate * dt) + 0.02 * accPitch;
  roll = 0.98 * (roll + gyroRollRate * dt) + 0.02 * accRoll;
  yaw = yaw + gyroYawRate * dt;
  if (yaw > 180) yaw -= 360;
  if (yaw < -180) yaw += 360;

  float calPitch = (pitch - pitchOffset) * INVERT_PITCH;
  float calRoll = (roll - rollOffset) * INVERT_ROLL;
  float calYaw = (yaw - yawOffset) * INVERT_YAW;
  if (calYaw > 180) calYaw -= 360;
  if (calYaw < -180) calYaw += 360;

  // ---- NEW: apply axis swap if enabled ----
  if (SWAP_PITCH_YAW) {
    float temp = calPitch;
    calPitch = calYaw;
    calYaw = temp;
  }

  // ---- Calibration button ----
  int currentButtonState = digitalRead(CALIBRATE_BTN);
  if (currentButtonState == LOW && lastButtonState == HIGH && (now - lastButtonPress > DEBOUNCE_DELAY)) {
    pitchOffset = pitch;
    rollOffset = roll;
    yawOffset = yaw;
    lastButtonPress = now;
    Serial.println(">>> CALIBRATED: Center reset <<<");
    flashBothLEDs(3);
  }
  lastButtonState = currentButtonState;

  // ---- Send data continuously ----
  if (now - lastSend >= SEND_INTERVAL) {
    String data = String(calYaw, 2) + "," + String(calPitch, 2) + "," + String(calRoll, 2);
    SerialBT.println(data);
    Serial.println(data);
    lastSend = now;
  }

  // ---- LED behavior with hysteresis (prevents flickering) ----
  bool pitchMoving = currentlyPitchMoving
                       ? (abs(calPitch) > EXIT_THRESHOLD)
                       : (abs(calPitch) > ENTER_THRESHOLD);

  bool sideMoving = currentlySideMoving
                      ? ((abs(calYaw) > EXIT_THRESHOLD) || (abs(calRoll) > EXIT_THRESHOLD))
                      : ((abs(calYaw) > ENTER_THRESHOLD) || (abs(calRoll) > ENTER_THRESHOLD));

  currentlyPitchMoving = pitchMoving;
  currentlySideMoving = sideMoving;

  if (pitchMoving) {
    digitalWrite(BLUE_LED, LOW);
    if (now - lastGreenBlink > GREEN_BLINK_SPEED) {
      greenState = !greenState;
      digitalWrite(GREEN_LED, greenState);
      lastGreenBlink = now;
    }
  } else if (sideMoving) {
    digitalWrite(BLUE_LED, HIGH);
    if (now - lastGreenBlink > GREEN_BLINK_SPEED) {
      greenState = !greenState;
      digitalWrite(GREEN_LED, greenState);
      lastGreenBlink = now;
    }
  } else {
    digitalWrite(BLUE_LED, HIGH);
    digitalWrite(GREEN_LED, HIGH);
  }
}