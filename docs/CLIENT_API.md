# CAN client reference

For installation, integration and submission, start with [README](../README.md).

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

See [PROTOCOL.md](../PROTOCOL.md) for exact CAN framing, retries and design tradeoffs. Portable protocol tests are in `tests/protocol_test.c`.
