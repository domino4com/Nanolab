#include <CanStatusClient.h>

// Keep this object global. CAN runs in the background after begin().
CanStatusClient canStatus;

void setup() {
  Serial.begin(115200);
  // Put your existing sensor initialization here. Do not initialize Wi-Fi/BLE.
  if (!canStatus.begin()) {
    Serial.println("CAN startup failed");
    return;
  }
  canStatus.setData("Starting");
}

void loop() {
  // Replace this sample with your actual measurements. Maximum 80 text bytes.
  char record[81];
  snprintf(record, sizeof(record), "temperature=%.2f status=OK", 23.5);
  canStatus.setData(record); // Copies the latest snapshot; no CAN loop call needed.
  delay(1000);
}
