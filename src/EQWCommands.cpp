#include "EQWCommands.h"
#include <map>
#include <string>

// Maps for user-defined commands
static std::map<std::string, uint8_t> userNameToId;
static std::map<uint8_t, std::string> userIdToName;
static uint8_t nextUserId = 0xEB;

uint8_t eqwCommandIdForName(const std::string& name) {
    for (const auto& entry : kEQWCommandTable) {
        if (name == entry.name) return entry.id;
    }
    auto it = userNameToId.find(name);
    if (it != userNameToId.end()) return it->second;
    return 0xFF;
}

const char* eqwCommandNameForId(uint8_t id) {
    for (const auto& entry : kEQWCommandTable) {
        if (id == entry.id) return entry.name;
    }
    auto it = userIdToName.find(id);
    if (it != userIdToName.end()) return it->second.c_str();
    return nullptr;
}

uint8_t defineUserCommand(const std::string& name) {
    uint8_t existing = eqwCommandIdForName(name);
    if (existing != 0xFF) return existing;
    if (nextUserId > 0xFD) return 0xFF; // Out of reserved IDs
    uint8_t id = nextUserId++;
    userNameToId[name] = id;
    userIdToName[id] = name;
    return id;
}
