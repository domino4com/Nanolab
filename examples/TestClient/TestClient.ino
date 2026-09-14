// Build with sketch.yaml: ESP32 core 3.3.11; Arduino IDE 2.3.10 / CLI 1.5.1.
// Use the adjacent compile/upload scripts. For your own sketch, see README.md.
#include <CanStatusClient.h>
CanStatusClient canStatus;
void setup() {
  Serial.begin(115200);
  delay(500);
  if (!canStatus.begin()) {
    Serial.println("CAN startup failed; check pins, driver ownership and configuration.");
    return;
  }
  canStatus.setData("Test Client ready");
}
void loop() {
  static uint32_t sequence=0, lastAck=0;
  uint32_t ack=canStatus.acknowledged();
  if (ack != lastAck) {
    Serial.printf("Server storage acknowledgments: %lu\n", (unsigned long)ack);
    lastAck=ack;
  }
  char text[81];
  snprintf(text,sizeof(text),"Test Client sample=%lu uptime_ms=%llu",(unsigned long)sequence++, (unsigned long long)(esp_timer_get_time()/1000));
  // Exercise all 80 bytes, including the final CAN fragment.
  size_t length = strlen(text);
  memset(text + length, '.', 80 - length);
  text[80] = 0;
  canStatus.setData(text);
  delay(1000); // No CAN polling function is needed in loop().
}
