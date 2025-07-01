# EQW Protocol Specification (v1)

## Overview

The EQW protocol is a compact, binary messaging format designed for ESP-NOW communication between ESP32-based devices. All packets follow a shared structure, supporting broadcast and peer-to-peer delivery, request/response correlation, and optional command acknowledgment.

---

## Packet Format

Every EQW packet starts with a fixed prefix and includes a flag-driven header followed by a payload:

| Byte Index | Field      | Type       | Description                                        |
|------------|------------|------------|----------------------------------------------------|
| 0          | Prefix[0]  | `0x45`     | `'E'`                                              |
| 1          | Prefix[1]  | `0x51`     | `'Q'`                                              |
| 2          | Prefix[2]  | `0x57`     | `'W'`                                              |
| 3          | Command ID | `uint8_t`  | Command being issued or replied to                 |
| 4          | Flags      | `uint8_t`  | Behavior of this packet (see below)                |
| 5–6        | Request ID | `uint16_t` | 0 = untracked, otherwise used for matching replies |
| 7–N        | Payload    | Variable   | Depends on command                                 |

**Maximum size:** 250 bytes (including all fields)

---

## Flag Byte Semantics

| Value  | Meaning         | Notes                                                                                                            |
|--------|-----------------|------------------------------------------------------------------------------------------------------------------|
| `0x00` | **Get** request | e.g., "What is your battery?"                                                                                    |
| `0x01` | **Reply**       | e.g., "Battery = 78%"                                                                                            |
| `0x02` | **Set** command | e.g., "Set brightness to 200"                                                                                    |
| `0x03` | **Set + Ack**   | e.g., "Set brightness to 200, confirm receipt"                                                                   |
| `0x04` | **Options**     | List of options or flags for the command, if applicable, e.g., "Light mode: Steady, Breathing, Pulsing, Rainbow" |

> Devices **must not** treat flags > 4 as valid unless explicitly documented in future protocol extensions.

---

## Command ID Assignment

| Command ID  | Meaning                                                              |
|-------------|----------------------------------------------------------------------|
| `0x00–0xEA` | EQW standard command space                                           |
| `0xEB–0xFF` | Reserved for user-defined commands                                   |

---

## System Commands: `SystemCommand` — Command ID `0x00`

### `QueryDevices` — Flag = `0x00`

Allows querying all devices or filtering by Device ID pairs (A+B) and/or specific MAC addresses.

#### Payload Format

| Byte Index | Field           | Type           | Notes                         |
|------------|-----------------|----------------|-------------------------------|
| 7          | Device Type ID Count | `uint8_t`      | N device type pairs to follow |
| 8..        | Device Type ID List  | `uint8_t[N*2]` | Each pair = A, B bytes        |
| ..         | MAC Count       | `uint8_t`      | M MACs follow                 |
| ..         | MACs            | `uint8_t[M*6]` | Each 6 bytes                  |

#### Example A: query all torches (A=0x01, B=0x02), no MACs
```
Full Payload:
  0x01       // 1 device type
  0x01 0x02  // Device ID: A=0x01, B=0x02
  0x00       // 0 MACs
```

#### Example B: query specific MAC 0xB0:0xB1:0xB2:0xB3:0xB4:0xB5
```
Full Payload:
  0x00                             // 0 device types
  0x01                             // 1 MAC
  0xB0 0xB1 0xB2 0xB3 0xB4 0xB5     // MAC bytes
```

---

### `SelfReport` — Flag = `0x01`

Sent as a reply to `QueryDevices`. Provides device metadata.

#### Payload Format

| Byte Index | Field         | Type         | Notes                      |
|------------|---------------|--------------|----------------------------|
| 7          | Device Byte A | `uint8_t`    | Creator/type ID upper byte |
| 8          | Device Byte B | `uint8_t`    | Device ID lower byte       |
| 9          | Version Major | `uint8_t`    | Semantic version           |
| 10         | Version Minor | `uint8_t`    |                            |
| 11         | Version Patch | `uint8_t`    |                            |
| 12         | Name Length   | `uint8_t`    | Max 32                     |
| 13..       | Name          | `char[N]`    | UTF-8-safe string          |
| +1         | Command Count | `uint8_t`    | How many commands follow   |
| +2         | Command IDs   | `uint8_t[M]` | List of supported commands |

#### Example: response from "Torch-3" with two commands supported
```
Full Payload:
  0x01 0x02                      // Device A + B
  0x01 0x00 0x07                 // Version: 1.0.7
  0x07                          // Name length = 7
  0x54 0x6F 0x72 0x63 0x68 0x2D 0x33   // 'T''o''r''c''h''-''3'
  0x02                          // 2 supported commands
  0x10 0x11                     // Command IDs
```

---

## Command Definition Examples

### `Battery` — ID: `0x06`

| Flag | Meaning      | Payload Format      |
|------|--------------|---------------------|
| 0x00 | GetBattery   | (no payload)        |
| 0x01 | BatteryReply | `uint8_t` battery % |

### `Brightness` — ID: `0x10`

| Flag | Meaning                     | Payload Format                                    |
|------|-----------------------------|---------------------------------------------------|
| 0x00 | GetBrightness               | (no payload)                                      |
| 0x01 | BrightnessReply             | `uint8_t` brightness (0–255)                      |
| 0x02 | SetBrightness               | `uint8_t` target brightness                       |
| 0x03 | SetBrightness + Acknowledge | `uint8_t` brightness, reply should echo requestId |

#### Example: Brightness GET
```
Header:
  0x45 0x51 0x57  0x10  0x00  0x12 0x34
Payload:
  (none)
```

#### Example: Brightness SET w/ ACK
```
Header:
  0x45 0x51 0x57  0x10  0x03  0xAB 0xCD
Payload:
  0xC8            // Brightness = 200
```

---

## Summary

The EQW protocol defines a 7-byte fixed header and flexible payload structure. Flags are used to distinguish between command types, and all packets are request-response compatible. System discovery and self-reporting operate via Command `0x00`, and all other functionality builds modularly on top of this structure.

Designed for real-time FreeRTOS use in LARP or embedded ESP-NOW environments.

---

## PlatformIO Library Usage

This repository includes a minimal reference implementation of the EQWNow library. Add it as a dependency in `platformio.ini`:

```ini
lib_deps =
    https://github.com/yourname/eqw-now.git
```

Include the header and initialize the library:

```cpp
#include <EQWNow.h>

EQWNow eqw;

void setup() {
    eqw.begin("DeviceName", 0x01, 0x02, 1, 0, 0);
}
```

The library automatically tracks commands registered via `on()` and advertises
them in its `SelfReport` reply to `QueryDevices`. Use `on("Power", handler)` to
register handlers by name or `on(0x01, handler)` for direct IDs. Besides
`send()` for fire-and-forget messages, `request()` allows sending a packet and
supplying a callback to handle the reply when it arrives. All incoming packets
are queued internally and processed in `EQWNow::process()` so callbacks execute
from your main loop rather than the Wi‑Fi driver task. Pending reply handlers
expire after a timeout (10\xa0seconds by default) which can be changed via
`setPendingReplyTimeout()`.
