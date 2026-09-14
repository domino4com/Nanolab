# Status client for Nanolab Extended Core

This Arduino library publishes the latest text snapshot when the Control server polls it. The server polls its configured clients every **60 seconds**, one at a time. The CAN protocol runs in a FreeRTOS task; no CAN processing call is needed in `loop()`.

## Install and use

1. Install Espressif's **esp32 Arduino core 3.3.8** or later.
2. Install the supplied library ZIP through Sketch → Include Library → Add .ZIP Library.
3. Select the ESP32-S3 board. Use ESP32S3 Dev Module, 8 MB flash, PSRAM disabled, USB mode Hardware CDC and JTAG, USB CDC On Boot enabled.
4. Add the following to your existing sketch. Keep the object global so it lives for the entire program.

```cpp
#include <CanStatusClient.h>
CanStatusClient canStatus;

void setup() {
  // Your existing sensor initialization goes here.
  if (!canStatus.begin()) {
    // Startup failed: GPIO40 turns red. Handle this for your application.
    return;
  }
  canStatus.setData("Starting");
}

void loop() {
  // After collecting a complete, consistent set of measurements:
  char record[81];
  snprintf(record, sizeof(record), "temperature=%.2f status=OK", 23.5);
  if (!canStatus.setData(record)) {
    // Rejected: longer than 80 bytes or contains control characters.
  }
  delay(1000); // Your existing application can do other work here.
}
```

Replace the example temperature with your actual measurements or data you want to sent. `setData()` copies the string immediately under a short lock, so a stack buffer is safe. It does not transmit immediately. The background task takes a coherent snapshot when polled. Updating once per second with a 60-second poll produces one logged latest snapshot per minute; intermediate measurements are not queued. This is suitable for status reporting, not lossless recording of every sensor measurement or brief alarm event.

The maximum is **80 bytes**, equivalent to 80 ASCII characters. UTF-8 characters may consume multiple bytes. CR, LF, tabs and other ASCII control characters are rejected to preserve one log record per line. Empty strings are allowed. Rejected updates leave the previous snapshot intact. Call from normal task context, not an ISR/Interupt. A client that has never received `setData()` replies with `NO_DATA`; snapshot age then equals `4294967295`.

## Options and API

```cpp
CanStatusClient::Options options;
options.txPin = 7;
options.rxPin = 6;
options.ledPin = 40;       // -1 disables library LED control
options.bitrate = 250000; // server default; alternatives 125000 or 500000
options.expectedId = 0x8dd4; // optional identity check; not an ID override
options.idleTimeoutMs = 180000;
bool started = canStatus.begin(options);
```

`begin()` returns true after driver/task creation, not proof of a functioning physical bus. Call it once. This library owns the ESP32-S3's classic TWAI controller; remove other CAN driver initialization. Do not destroy the object while running. `nodeId()` returns its derived ID; `running()` reports task creation; `acknowledged()` counts received storage acknowledgments since boot. There is no dynamic stop/reconfigure API.

`begin()` executes:

```cpp
esp_wifi_stop();
esp_wifi_deinit();
esp_bt_controller_disable();
esp_bt_controller_deinit();
```

The Bluetooth calls are compiled only when the core enables the Bluetooth controller; otherwise it is already unavailable. Not-initialized/disabled return codes are expected and ignored. The library never starts Wi-Fi or BLE. Remove Wi-Fi/BLE startup from the rest of your sketch: another component can explicitly restart radios after these shutdown calls. This is a startup shutdown, not enforcement against later application code. `CanStatusClient::disableRadios()` can also be called explicitly.

## LED indications

Client: blue during initialization; amber while awaiting a poll; cyan while responding; green after the server acknowledges storage; red on startup failure or CAN bus-off. After three minutes without a poll it returns to amber. Colors use low brightness. Green means an acknowledgment was received, not that the connection is continuously checked between polls.

Server: blue at startup; cyan after a valid RTC read; amber while waiting for clients; green when a client has responded; red for CAN, RTC, SD or deferred-storage faults. `status.txt` gives per-client results, so a green LED does not mean every configured client is online.

## Files and time

`data8dd4.txt` receives tab-separated, newline-terminated records such as:

```text
2026-09-11T12:34:56Z	server_uptime_s=60.125	client_uptime_s=72.403	sample_age_ms=403	token=12345678	status=OK	data=temperature=23.50 status=OK
```

The server appends only CRC-validated complete replies. It closes the file before sending the application acknowledgment. While SFTP holds a log open, FatFS locking may prevent append: the server retains one sample per client in RAM and retries storage, skipping further polls of that client until it succeeds. Other clients continue. `status.txt` exposes pending storage and timeouts. Do not upload over active `data*.txt` logs. Power loss can lose a pending sample or damage a FAT write; this is not a transactional database. A failed partial SD write can leave an incomplete record or a duplicate retry, identified by the same token. Download/close files promptly instead of keeping handles open indefinitely.

The example `TestClient` intentionally pads its status to 80 bytes with dots to exercise the maximum message size. For real projects, use the small integration example above without padding.

See `PROTOCOL.md` for exact CAN framing, retries and design tradeoffs. Portable protocol tests are in `tests/protocol_test.c`.
