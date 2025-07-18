#ifndef EQW_COMMANDS_H
#define EQW_COMMANDS_H

#include <stdint.h>
#include <string>

struct EQWCommandEntry {
    const char* name;
    uint8_t id;
};

static const EQWCommandEntry kEQWCommandTable[] = {
    {"SystemCommand", 0x00},
    {"Power", 0x01},
    {"Reboot", 0x02},
    {"Reset", 0x03},
    {"Setting", 0x04},
    {"Action", 0x05},
    {"Battery", 0x06},
    {"Brightness", 0x10},
    {"RGBColor", 0x11},
    {"RGBWColor", 0x12},
    {"AnimationMode", 0x13},
    {"AnimationSpeed", 0x14},
    {"WiFiCredentials", 0x25},
    {"APCredentials", 0x26},
    {"IPAddress", 0x27},
    {"AudioPlay", 0x35},
    {"AudioStop", 0x36},
    {"Volume", 0x37},
    {"AudioList", 0x38},
    {"Sensors", 0x70},
    {"SDSpace", 0x91},
    {"FileList", 0x92},
    {"Macro", 0xC8},

};

uint8_t eqwCommandIdForName(const std::string& name);
const char* eqwCommandNameForId(uint8_t id);
uint8_t defineUserCommand(const std::string& name);

#endif // EQW_COMMANDS_H
