// ===== MPU6050/6500 TEST (raw I2C, works with clone chips) =====
// Wiring: VCC->3V3, GND->GND, SDA->D21, SCL->D22

#include <Wire.h>

#define BLUE_LED 25
#define GREEN_LED 26
#define MPU_ADDR 0x68

int16_t ax, ay, az, gx, gy, gz;
float pitch = 0, roll = 0;
float lastPitch = 0, lastRoll = 0;
unsigned long lastTime;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== MPU TEST START ===");

  pinMode(BLUE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(GREEN_LED, LOW);

  Wire.begin(21, 22);

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  delay(100);
  Serial.println("Sensor initialized.");
  digitalWrite(GREEN_LED, HIGH);

  lastTime = millis();
}

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

void loop() {
  readMPU();

  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0;
  lastTime = now;

  float accPitch = atan2(ay, az) * 180.0 / PI;
  float accRoll  = atan2(ax, az) * 180.0 / PI;
  float gyroPitchRate = gx / 131.0;
  float gyroRollRate  = gy / 131.0;

  pitch = 0.98 * (pitch + gyroPitchRate * dt) + 0.02 * accPitch;
  roll  = 0.98 * (roll + gyroRollRate * dt) + 0.02 * accRoll;

  Serial.print("Pitch: ");
  Serial.print(pitch);
  Serial.print("   Roll: ");
  Serial.println(roll);

  float pitchChange = abs(pitch - lastPitch);
  float rollChange  = abs(roll - lastRoll);

  digitalWrite(BLUE_LED, (pitchChange > 2.0 || rollChange > 2.0) ? HIGH : LOW);

  lastPitch = pitch;
  lastRoll = roll;

  delay(50);
}