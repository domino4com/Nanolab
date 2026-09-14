# CAN status protocol v1

## Why server polling

A single server polling 15 status clients once per minute gives bounded traffic, predictable storage, and explicit offline detection. Clients expose their latest snapshot without coordinating transmission schedules. An 80-byte response takes 25 classic CAN frames; even 15 maximum-size responses per minute leave ample capacity at 250 kbit/s. Polling is appropriate for current status. If every intermediate measurement or immediate alarm must be retained, add client queues/event messages and persistence rather than treating this snapshot protocol as lossless telemetry.

ESP32-S3 TWAI supports classic CAN with 8-byte payloads and 29-bit identifiers. This implementation uses a small custom segmented transport because this is a closed network under one application's control. CANopen or ISO-TP would be preferable if integration with existing industrial devices/tools becomes a requirement. Sources: [Espressif ESP32-S3 TWAI documentation](https://docs.espressif.com/projects/esp-idf/en/v5.4.4/esp32s3/api-reference/peripherals/twai.html), [DS3231 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/ds3231.pdf).

## Wire format

Classic CAN, 250000 bit/s, extended identifier, DLC 8, no remote frames. Every integer inside a payload is little-endian. CAN identifier:

```text
0x1A000000 | (type << 22) | (node_id << 6) | fragment_index
```

| Bits | Meaning |
|---|---|
| 28..25 | namespace 0b1101 |
| 24..22 | frame type |
| 21..6 | 16-bit MAC suffix/node ID |
| 5..0 | fragment index |

| Type | Direction | Index | Eight payload bytes |
|---:|---|---:|---|
| 0 POLL | server → client | 0 | version=1, reserved=0, token u32, reserved=0, reserved=0 |
| 1 IDENTITY | client → server | 0 | token u32, first four MAC bytes |
| 2 HEADER | client → server | 0 | token u32, text length u8, status u8, CRC16 u16 |
| 3 DATA | client → server | 0..22 | token u32, four body bytes |
| 4 ACK | server → client | 0 | token u32, result=0, reserved zero ×3 |

Reconstruct MAC bytes 4 and 5 from the high and low bytes of `node_id`. Status 0 is OK; status 1 is NO_DATA and requires text length zero. Text length is 0..80 bytes. Body:

| Offset | Size | Meaning |
|---:|---:|---|
| 0 | 8 | client uptime milliseconds, uint64 |
| 8 | 4 | age of published snapshot in milliseconds, uint32 |
| 12 | 0..80 | text bytes, no NUL terminator |

Number of DATA frames: ceiling((12 + text length)/4). Pad the last frame with zeros; padding is excluded from CRC. CRC-16/CCITT-FALSE: polynomial 0x1021, initial 0xFFFF, no reflection, no final XOR. CRC input is full MAC (6 bytes), text length (1), status (1), then the unpadded body. Standard test vector `123456789` yields 0x29B1. Token validation separates transactions; CAN's own frame CRC protects each frame in addition to the application CRC over the complete response.

## Transaction and recovery

1. Server selects the next configured client and assigns a 32-bit token (random starting value per boot, then incremented).
2. Client takes an atomic copy of its latest published text and metadata, caches it under that token, then sends IDENTITY, HEADER, DATA in ascending index order.
3. Server accepts frames only for the expected ID/token, reassembles them, verifies fields and CRC, and rejects malformed or conflicting duplicate fragments. Out-of-order fragments and identical duplicates are supported.
4. Server appends the received record and closes the file, then sends ACK. An ACK is an application storage acknowledgment, distinct from CAN's hardware acknowledgment bit.
5. Missing/incomplete/bad replies receive up to three total attempts, each with a 500 ms timeout and the **same token**. A repeated token causes the client to resend the exact cached snapshot, not a newer measurement.
6. After failure, the client is marked offline in status, and polling continues to the next client. There is no fake data record for a timeout. Next normal round starts 60 seconds after the preceding round started.

Only one request is outstanding at a time. Pending SD writes retain the completed record per node, so a file locked by SFTP does not make the server request a replacement snapshot. There is no permanent exactly-once guarantee across power failure. A lost ACK does not trigger client retransmission by itself; it leaves the client's LED cyan until a later successful transaction. Frames use single-shot transmission and bounded application retries, avoiding unlimited hardware retry loops. Both endpoints initiate standard bus-off recovery and restart TWAI afterward.

This private wired protocol provides integrity/error detection, not cryptographic authentication. Keep the bus under your physical control. SFTP separately protects access over USB networking.
