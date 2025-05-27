# EQW Library Usage and Lifecycle Design

This document defines the intended lifecycle and architectural responsibilities of the EQWNow library, including initialization, handler registration, self-reporting, and peer management. This specification is implementation-agnostic and focuses on expected behaviors and interactions.

---

## Initialization (`begin()` or equivalent)

The initialization method must:

- Set up ESP-NOW and register send/receive callbacks.
- Accept parameters including:
  - A human-readable device name (max 32 bytes).
  - Device identifier bytes A and B.
  - Semantic version (major, minor, patch).
- Internally capture the list of supported commands based on registered handlers.
- Automatically prepare to reply to incoming `QueryDevices` with `SelfReport`, using stored values and registered handlers.
- Register a periodic or on-demand loop handler to allow processing of delayed tasks (e.g. response timeouts).

Self-report metadata must always include:
- MAC address (from ESP-NOW layer, not payload).
- Device A + B.
- Version tuple.
- Name (UTF-8, 32 bytes max).
- List of supported command IDs (as registered).

---

## Command Execution (`send()` / `sendWithResponse()`)

Command execution may be broadcast or sent to a specific peer. It must:

- Allow fire-and-forget commands (requestId = 0).
- Allow commands to request a reply by assigning a unique requestId.
- Encode all outgoing messages using the 7-byte protocol header followed by raw payload.
- Handle responses (if expected) via:
  - Optional async callback handler.
  - Or await mechanism with timeout.

Replies must match the requestId and sender MAC.

---

## Handler Registration (`on("CommandName", handler)`)

Handler registration APIs should:

- Map string names to command ID bytes via a centralized command registry.
- Automatically record the command ID as a supported command for this device.
- Ensure thread/task safety when invoking handlers.

Handlers may be invoked with:
- Raw payload bytes.
- Request metadata (e.g. sender MAC, requestId if applicable).

Command ID space is limited to 255 values. A portion of the range (e.g. final 20 IDs) must be reserved for future **user-defined commands**, via a dedicated definition option (to be implemented later).

---

## MAC Tracking and Peer Discovery

The library must:

- Support receiving and parsing `SelfReport` packets in response to a `QueryDevices` broadcast.
- Require the developer to explicitly **store** a discovered peer using a method like `.storeAsPeer()`.
- Maintain a persistent list of stored peers, including:
  - MAC address
  - Name
  - Device A + B
  - Version tuple
  - Supported commands
- Provide an API to enumerate stored peers for later use.
- Support sending `QueryDevices` packets with optional filters (Device ID, MAC).
- Support discovery timeout + response aggregation.

Peers **must not** be automatically cached — this is opt-in via explicit `.storeAsPeer()` calls.

---

## Automatic SelfReport Integration

When handlers are registered via `.on()`, the EQWNow instance must:

- Track those commands as part of the capabilities advertised in `SelfReport`.
- Ensure that only **registered and allowed commands** are included.

A central command registry should:
- Map command strings to byte values.
- Maintain a reverse lookup for byte → string.
- Reserve the final 20 byte values (e.g. `0xEC–0xFF`) for user-defined entries via a future interface.

This guarantees that discovery of a device reflects its actual, runtime-capable command set.

---

This design ensures predictability, modularity, and introspection across LARP devices using the EQW protocol.
