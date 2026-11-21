// ===== VR HEADSET MODULE - v3 (directional LED feedback + calibration button) =====
// Blue LED: blinks fast = pitch (up/down), solid ON = yaw/roll (side/tilt), OFF = still
// Green LED: solid ON once = system ready
// MPU6050/6500 (SDA=D21, SCL=D22) + Bluetooth
// Calibration Button: D33 -> press once to reset Yaw/Pitch/Roll to 0 (set "forward" position)

#include <Wire.h>
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

#define BLUE_LED 25
#define GREEN_LED 26
#define MPU_ADDR 0x68
#define CALIBRATE_BTN 33

int16_t ax, ay, az, gx, gy, gz;
float pitch = 0, roll = 0, yaw = 0;
float lastPitch = 0, lastRoll = 0, lastYaw = 0;

unsigned long lastTime = 0;
unsigned long lastSend = 0;
unsigned long lastMovementCheck = 0;
unsigned long lastBlinkToggle = 0;
unsigned long lastButtonPress = 0;

int movementMode = 0;   // 0 = still, 1 = pitch (blink), 2 = yaw/roll (solid)

const float MOVEMENT_THRESHOLD = 0.5;   // degrees
const int SEND_INTERVAL = 20;           // ms
const int CHECK_INTERVAL = 150;         // ms
const int BLINK_SPEED = 80;             // ms between blink toggles
const int DEBOUNCE_DELAY = 300;         // ms, prevents multiple triggers per press

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
Serial.println("=== VR HEADSET MODULE START ===");

pinMode(BLUE_LED, OUTPUT);
pinMode(GREEN_LED, OUTPUT);
digitalWrite(BLUE_LED, LOW);
digitalWrite(GREEN_LED, LOW);

pinMode(CALIBRATE_BTN, INPUT_PULLUP);

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

Serial.println("MPU sensor initialized. Streaming data now...");
digitalWrite(GREEN_LED, HIGH);

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

  // ---- Calibration button: press to reset Yaw/Pitch/Roll to 0 ----
  if (digitalRead(CALIBRATE_BTN) == LOW && (now - lastButtonPress > DEBOUNCE_DELAY)) {
    yaw = 0;
    pitch = 0;
    roll = 0;
    Serial.println("CALIBRATED: Yaw/Pitch/Roll reset to 0");
    lastButtonPress = now;
  }

  // ---- Send data continuously (moving or still) ----
if (now - lastSend >= SEND_INTERVAL) {
    String data = String(yaw, 2) + "," + String(pitch, 2) + "," + String(roll, 2);
SerialBT.println(data);
Serial.println(data);
    lastSend = now;
  }

  // ---- Detect movement type every CHECK_INTERVAL ms ----
if (now - lastMovementCheck >= CHECK_INTERVAL) {
float pitchChange = abs(pitch - lastPitch);
float rollChange  = abs(roll - lastRoll);
float yawChange   = abs(yaw - lastYaw);

bool pitchMoving = pitchChange > MOVEMENT_THRESHOLD;
bool sideMoving  = (rollChange > MOVEMENT_THRESHOLD) || (yawChange > MOVEMENT_THRESHOLD);

if (pitchMoving) {
      movementMode = 1;
    } else if (sideMoving) {
      movementMode = 2;
    } else {
      movementMode = 0;
    }

    lastPitch = pitch;
    lastRoll = roll;
    lastYaw = yaw;
    lastMovementCheck = now;
  }

  // ---- Apply LED behavior ----
if (movementMode == 1) {
if (now - lastBlinkToggle > BLINK_SPEED) {
digitalWrite(BLUE_LED, !digitalRead(BLUE_LED));
      lastBlinkToggle = now;
    }
  } else if (movementMode == 2) {
digitalWrite(BLUE_LED, HIGH);
  } else {
digitalWrite(BLUE_LED, LOW);
  }
}