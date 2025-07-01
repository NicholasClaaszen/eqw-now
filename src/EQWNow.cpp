#include "EQWNow.h"
#include "EQWCommands.h"

EQWNow* EQWNow::instance = nullptr;

EQWNow::EQWNow() {
    instance = this;
}

bool EQWNow::begin(const char* name,
                   uint8_t deviceA,
                   uint8_t deviceB,
                   uint8_t verMajor,
                   uint8_t verMinor,
                   uint8_t verPatch) {
    strncpy(info.name, name, EQW_MAX_NAME_LEN);
    info.deviceByteA = deviceA;
    info.deviceByteB = deviceB;
    info.versionMajor = verMajor;
    info.versionMinor = verMinor;
    info.versionPatch = verPatch;

    if (esp_now_init() != ESP_OK) {
        return false;
    }

    esp_now_register_recv_cb(EQWNow::onDataRecv);
    esp_now_register_send_cb(EQWNow::onDataSent);

    on("SystemCommand",
       [this](const uint8_t* mac, const uint8_t* payload, size_t len, uint8_t flag, uint16_t req) {
           handleSystemCommand(mac, payload, len, flag, req);
       });

    registeredCommands.insert(0x00);

    return true;
}

void EQWNow::on(uint8_t commandId, ReceiveCallback cb) {
    callbacks[commandId] = cb;
    registeredCommands.insert(commandId);
}

void EQWNow::on(const char* commandName, ReceiveCallback cb) {
    uint8_t id = eqwCommandIdForName(commandName);
    if (id != 0xFF) {
        on(id, cb);
    }
}

bool EQWNow::send(const uint8_t* mac,
                  uint8_t commandId,
                  uint8_t flag,
                  const uint8_t* payload,
                  size_t len,
                  uint16_t requestId) {
    if (!mac || len > 243) {
        return false;
    }

    uint8_t packet[250];
    size_t idx = 0;
    packet[idx++] = 0x45;
    packet[idx++] = 0x51;
    packet[idx++] = 0x57;
    packet[idx++] = commandId;
    packet[idx++] = flag;
    packet[idx++] = requestId >> 8;
    packet[idx++] = requestId & 0xFF;

    memcpy(packet + idx, payload, len);
    idx += len;

    return esp_now_send(mac, packet, idx) == ESP_OK;
}

void EQWNow::process() {
    // placeholder for async tasks
}

bool EQWNow::sendSelfReport(const uint8_t* mac, uint16_t requestId) {
    uint8_t payload[256];
    size_t idx = 0;
    payload[idx++] = info.deviceByteA;
    payload[idx++] = info.deviceByteB;
    payload[idx++] = info.versionMajor;
    payload[idx++] = info.versionMinor;
    payload[idx++] = info.versionPatch;
    uint8_t nameLen = strnlen(info.name, EQW_MAX_NAME_LEN);
    payload[idx++] = nameLen;
    memcpy(payload + idx, info.name, nameLen);
    idx += nameLen;
    payload[idx++] = registeredCommands.size();
    for (uint8_t cmd : registeredCommands) {
        payload[idx++] = cmd;
    }
    return send(mac, 0x00, 0x01, payload, idx, requestId);
}

void EQWNow::handleSystemCommand(const uint8_t* mac,
                                 const uint8_t* payload,
                                 size_t len,
                                 uint8_t flag,
                                 uint16_t requestId) {
    if (flag == 0x00) {
        sendSelfReport(mac, requestId);
    }
}

void EQWNow::onDataRecv(const uint8_t* mac, const uint8_t* data, int len) {
    if (!instance || len < 7) return;

    uint8_t commandId = data[3];
    uint8_t flag = data[4];
    uint16_t requestId = (data[5] << 8) | data[6];
    auto it = instance->callbacks.find(commandId);
    if (it != instance->callbacks.end()) {
        it->second(mac, data + 7, len - 7, flag, requestId);
    } else if (commandId == 0x00) {
        instance->handleSystemCommand(mac, data + 7, len - 7, flag, requestId);
    }
}

void EQWNow::onDataSent(const uint8_t* mac, esp_now_send_status_t status) {
    // stub for future ack handling
}

