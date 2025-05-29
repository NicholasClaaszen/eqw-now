# EQW Command List

This document defines the reserved and assigned command IDs for the EQW protocol.

## Command ID Allocation

- `0x00 - 0xEA` (0 - 234): Reserved for EQW standard commands
- `0xEB - 0xFF` (235 - 255): Reserved for future **user-defined** commands

Note: All commands may support multiple request types using the flag byte:
- `0x00`: GET
- `0x01`: REPLY (implied by GET)
- `0x02`: SET
- `0x03`: SET + ACK
- `0x04`: OPTIONS

---

## Group: Core Commands

| ID   | Name          | Description                         | GET | SET | OPTIONS |
|------|---------------|-------------------------------------|-----|-----|---------|
| 0x00 | SystemCommand | QueryDevices / SelfReport multiplex | ✅   | ✅   |         |
| 0x01 | Power         | Get/set device state on/off         | ✅   | ✅   |         |
| 0x02 | Reboot        | Reboot the device                   |     | ✅   |         |
| 0x03 | Reset         | Reset settings and reboot           |     | ✅   |         |
| 0x04 | Setting       | Generic key/value access            | ✅   | ✅   | ✅       |
| 0x05 | Action        | Named non-setting actions (trigger) |     | ✅   | ✅       |
| 0x06 | Battery       | Return battery percentage           | ✅   |     |         |

---

## Group: Lighting and Animation (Optional)

| ID   | Name           | Description                                                                                             | GET | SET | OPTIONS |
|------|----------------|---------------------------------------------------------------------------------------------------------|-----|-----|---------|
| 0x10 | Brightness     | Get/set brightness (0–255)                                                                              | ✅   | ✅   |         |
| 0x11 | RGBColor       | Set current color with RGB payload, optional 4th byte for interface, options lists available interfaces | ✅   | ✅   | ✅        |
| 0x12 | RGBWColor      | Set color with RGBW payload, optional 5th byte for interface, options lists available interfaces                                            | ✅   | ✅   | ✅        |
| 0x13 | AnimationMode  | Get/set animation mode                                                                                  | ✅   | ✅   | ✅       |
| 0x14 | AnimationSpeed | Get/set animation speed                                                                                 | ✅   | ✅   |         |

---

## Group: Network (Optional)

| ID   | Name            | Description                                         | GET | SET | OPTIONS |
|------|-----------------|-----------------------------------------------------|-----|-----|---------|
| 0x25 | WiFiCredentials | Set SSID/password and reboot, Get returns SSID only | ✅   | ✅   |         |
| 0x26 | APCredentials   | Set SSID/password and reboot, Get returns SSID only | ✅   | ✅   |         |
| 0x27 | IPAddress       | Get IP Address                                      | ✅   |     |         |

---

## Group: Audio (Optional)

| ID   | Name      | Description                 | GET | SET | OPTIONS |
|------|-----------|-----------------------------|-----|-----|---------|
| 0x35 | AudioPlay | Play an audio file by name  |     | ✅   |         |
| 0x36 | AudioStop | Stop current audio playback |     | ✅   |         |
| 0x37 | Volume    | Get/set volume              | ✅   | ✅   |         |
| 0x38 | AudioList | Return list of audio files  | ✅   |     |         |

---

## Group: Sensors and Status (Optional)

| ID   | Name     | Description                                                | GET | SET | OPTIONS |
|------|----------|------------------------------------------------------------|-----|-----|---------|
| 0x70 | Sensors  | Get sensor data (temp, humidity, etc.) Use OPTIONS to list | ✅   |     | ✅       |
| 0x91 | SDSpace  | Return available/used SD card space                        | ✅   |     |         |
| 0x92 | FileList | List files on SD                                           | ✅   |     |         |

---

## Group: Macros and Shortcuts (Optional)

| ID   | Name  | Description              | GET | SET | OPTIONS |
|------|-------|--------------------------|-----|-----|---------|
| 0xC8 | Macro | List and activate macros | ✅   | ✅   | ✅       |

---

## Notes

- Commands are listed once; use flag byte to distinguish GET/SET/etc.
- Only commands explicitly registered via `.on("Name")` or `.onCommandId()` will appear in `SelfReport`.
- Command registry must include both forward (string → byte) and reverse mappings.
- `0xEB–0xFD` is reserved for user-extended commands via a `.defineUserCommand()` interface.
- No file upload exists in EQW; audio and SD file interaction is read/delete only.
