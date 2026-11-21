// ===== BLUETOOTH TEST =====
// Pairs as "VR_Headset_Test", sends a counter message once per second

#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== BLUETOOTH TEST START ===");

  if (SerialBT.begin("VR_Headset_Test")) {
    Serial.println("Bluetooth started. Device name: VR_Headset_Test");
  } else {
    Serial.println("Bluetooth FAILED to start.");
  }
}

void loop() {
  static int counter = 0;
  String msg = "Hello from ESP32 #" + String(counter);

  Serial.println(msg);
  SerialBT.println(msg);

  counter++;
  delay(1000);
}