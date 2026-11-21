// ===== VR HEADSET MODULE - Mark-1 =====
// LED (Blue=D25, Green=D26) + MPU6050/6500 (SDA=D21, SCL=D22) + Bluetooth

#include <Wire.h>
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

#define BLUE_LED 25
#define GREEN_LED 26
#define MPU_ADDR 0x68

int16_t ax, ay, az, gx, gy, gz;
float pitch = 0, roll = 0, yaw = 0;
float lastPitch = 0, lastRoll = 0;
unsigned long lastTime;
unsigned long lastSend = 0;

void readMPU() {
Wire.beginTransmission(MPU_ADDR);
Wire.write(0x3B);
Wire.endTransmission(false);
Wire.requestFrom(MPU_ADDR, 14, true);

  ax = Wire.read() << 8 | Wire.read();
  ay = Wire.read() << 8 | Wire.read();
  az = Wire.read() << 8 | Wire.read();
Wire.read(); Wire.read();  // skip temperature
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
digitalWrite(BLUE_LED, HIGH);   // Blue ON = powered on
digitalWrite(GREEN_LED, LOW);

  // Start Bluetooth
if (SerialBT.begin("VR_Headset")) {
Serial.println("Bluetooth started: VR_Headset");
  } else {
Serial.println("Bluetooth FAILED to start.");
  }

  // Start I2C + wake MPU6050/6500
Wire.begin(21, 22);
Wire.beginTransmission(MPU_ADDR);
Wire.write(0x6B);   // PWR_MGMT_1 register
Wire.write(0);      // wake up
Wire.endTransmission(true);
delay(100);

Serial.println("MPU sensor initialized.");
digitalWrite(GREEN_LED, HIGH);   // Green ON = sensor ready

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

  // Send data over Bluetooth as "Y,P,R" every ~20ms (50 times/sec)
if (now - lastSend > 20) {
    String data = String(yaw) + "," + String(pitch) + "," + String(roll);
SerialBT.println(data);
Serial.println(data);
    lastSend = now;
  }

  // Blue LED blinks briefly to show movement detected
float pitchChange = abs(pitch - lastPitch);
float rollChange  = abs(roll - lastRoll);
digitalWrite(BLUE_LED, (pitchChange > 2.0 || rollChange > 2.0) ? HIGH : LOW);

  lastPitch = pitch;
  lastRoll = roll;
}

