# EQW Protocol Specification (v1)

## Overview

The EQW protocol is a compact, binary messaging format designed for ESP-NOW communication between ESP32-based devices. All packets follow a shared structure, supporting broadcast and peer-to-peer delivery, request/response correlation, and optional command acknowledgment.

---

## Packet Format

Every EQW packet starts with a fixed prefix and includes a flag-driven header followed by a payload:

| Byte Index | Field       | Type       | Description |
|------------|-------------|------------|-------------|
| 0          | Prefix[0]   | `0x45`     | `'E'`       |
| 1          | Prefix[1]   | `0x51`     | `'Q'`       |
| 2          | Prefix[2]   | `0x57`     | `'W'`       |
| 3          | Command ID  | `uint8_t`  | Command being issued or replied to |
| 4          | Flags       | `uint8_t`  | Behavior of this packet (see below) |
| 5–6        | Request ID  | `uint16_t` | 0 = untracked, otherwise used for matching replies |
| 7–N        | Payload     | Variable   | Depends on command |

**Maximum size:** 250 bytes (including all fields)

---

## Flag Byte Semantics

| Bit Pattern | Meaning                     | Notes                            |
|-------------|-----------------------------|----------------------------------|
| `0x00`      | **Get** request             | e.g., "What is your battery?"    |
| `0x01`      | **Reply**                   | e.g., "Battery = 78%"            |
| `0x02`      | **Set** command             | e.g., "Set brightness to 200"    |
| `0x03`      | **Set + Ack**               | e.g., "Set brightness to 200, confirm receipt" |

> Devices **must not** treat flags > 3 as valid unless explicitly documented in future protocol extensions.

---

## Command ID Assignment

| Command ID | Meaning                     |
|------------|-----------------------------|
| `0x00–0xFE`| User-defined command space   |
| `0xFF`     | Reserved system command:
   - `QueryDevices` (flag = 0)
   - `SelfReport` (flag = 1)

---

## System Commands: `0xFF` (QueryDevices & SelfReport)

### `QueryDevices` — Command ID `0xFF`, Flag = `0x00`

| Byte Index | Field            | Type       | Notes                      |
|------------|------------------|------------|----------------------------|
| 7          | Device ID Count  | `uint8_t`  | N device type pairs to follow |
| 8..        | Device ID List   | `uint8_t[N*2]` | Each pair = A, B bytes |
| ..         | MAC Count        | `uint8_t`  | M MACs follow             |
| ..         | MACs             | `uint8_t[M*6]` | Each 6 bytes             |

If both lists are empty, the query is broadcast to all devices.

#### Example: query all torches (A=0x01, B=0x02), no MACs
```
Command ID: 0xFF
Flag: 0x00
Request ID: 0x1234
Payload:
01 // 1 device type
01 02 // Device ID (creator/device)
00 // 0 MACs
```


---

### `SelfReport` — Command ID `0xFF`, Flag = `0x01`

| Byte Index | Field             | Type        | Notes |
|------------|-------------------|-------------|-------|
| 7          | Device Byte A     | `uint8_t`   | Creator/type ID upper byte |
| 8          | Device Byte B     | `uint8_t`   | Device ID lower byte       |
| 9          | Version Major     | `uint8_t`   | Semantic version           |
| 10         | Version Minor     | `uint8_t`   |                             |
| 11         | Version Patch     | `uint8_t`   |                             |
| 12         | Name Length       | `uint8_t`   | Max 32                     |
| 13..       | Name              | `char[N]`   | UTF-8-safe string          |
| +1         | Command Count     | `uint8_t`   | How many commands follow   |
| +2         | Command IDs       | `uint8_t[M]`| List of supported commands |

#### Example: response from `"Torch-3"` with two commands supported

```
Command ID: 0xFF
Flag: 0x01
Request ID: 0x1234
Payload:
01 02 // Device A + B
01 00 07 // Version 1.0.7
07 // Name length
'T' 'o' 'r' 'c' 'h' '-' '3'
02 // Two supported commands
0x10 0x11 // Command IDs
```


---

## Command Definition Example

### `BatteryStatus` — ID: `0x10`

| Flag | Meaning                       | Payload Format |
|------|-------------------------------|----------------|
| 0x00 | GetBattery                    | (no payload)   |
| 0x01 | BatteryReply                  | `uint8_t` battery % |
| 0x02 | Invalid                       | Not defined    |
| 0x03 | Invalid                       | Not defined    |

### `Brightness` — ID: `0x11`

| Flag | Meaning                       | Payload Format |
|------|-------------------------------|----------------|
| 0x00 | GetBrightness                 | (no payload)   |
| 0x01 | BrightnessReply               | `uint8_t` brightness (0–255) |
| 0x02 | SetBrightness                 | `uint8_t` target brightness |
| 0x03 | SetBrightness + Acknowledge   | `uint8_t` brightness, reply should echo requestId |

#### Example: Brightness GET
```
EQW 0x11 0x00 0x12 0x34
(no payload)
```


#### Example: Brightness SET w/ ACK
```
EQW 0x11 0x03 0xAB 0xCD 0xC8
// Set brightness to 200, expect reply with same requestId
```


---

## Summary

The EQW protocol defines a 7-byte fixed header and flexible payload structure. Flags are used to distinguish between command types, and all packets are request-response compatible. System commands use `Command ID = 255`, and the protocol is designed for size-constrained, real-time embedded use with FreeRTOS in ESP-NOW environments.
