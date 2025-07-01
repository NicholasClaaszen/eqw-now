#include "EQWNow.h"
#include "EQWCommands.h"

static const uint8_t kEQWPrefix[3] = {0x45, 0x51, 0x57};

EQWNow* EQWNow::instance = nullptr;

static std::array<uint8_t, 6> macToArray(const uint8_t* mac) {
    std::array<uint8_t, 6> arr{};
    if (mac) memcpy(arr.data(), mac, 6);
    return arr;
}

EQWNow::EQWNow() { instance = this; }

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

    rxQueue = xQueueCreate(10, sizeof(QueuedMessage));
    if (!rxQueue) {
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
    packet[idx++] = kEQWPrefix[0];
    packet[idx++] = kEQWPrefix[1];
    packet[idx++] = kEQWPrefix[2];
    packet[idx++] = commandId;
    packet[idx++] = flag;
    packet[idx++] = requestId >> 8;
    packet[idx++] = requestId & 0xFF;

    memcpy(packet + idx, payload, len);
    idx += len;

    return esp_now_send(mac, packet, idx) == ESP_OK;
}

uint16_t EQWNow::request(const uint8_t* mac,
                         uint8_t commandId,
                         uint8_t flag,
                         const uint8_t* payload,
                         size_t len,
                         ReceiveCallback replyCb) {
    uint16_t id = nextRequestId++;
    if (id == 0) id = nextRequestId++; // avoid 0
    if (!send(mac, commandId, flag, payload, len, id)) {
        return 0;
    }
    PendingReply pr;
    pr.cb = replyCb;
    pr.timestamp = millis();
    pendingReplies[id] = pr;
    return id;
}

uint16_t EQWNow::queryDevices(const uint8_t* mac,
                              const std::vector<std::pair<uint8_t, uint8_t>>& deviceIds,
                              const std::vector<std::array<uint8_t, 6>>& macs) {
    uint8_t payload[64];
    size_t idx = 0;
    payload[idx++] = deviceIds.size();
    for (const auto& p : deviceIds) {
        payload[idx++] = p.first;
        payload[idx++] = p.second;
    }
    payload[idx++] = macs.size();
    for (const auto& m : macs) {
        memcpy(payload + idx, m.data(), 6);
        idx += 6;
    }
    return request(mac, 0x00, 0x00, payload, idx,
                   [this](const uint8_t* mac, const uint8_t* payload, size_t len, uint8_t flag, uint16_t id) {
                       handleSystemCommand(mac, payload, len, flag, id);
                   });
}

void EQWNow::onSelfReport(SelfReportCallback cb) { selfReportCb = cb; }

void EQWNow::storePeer(const EQWPeerRecord& peer) {
    peers[macToArray(peer.mac)] = peer;
}

bool EQWNow::getPeer(const uint8_t* mac, EQWPeerRecord& out) const {
    auto it = peers.find(macToArray(mac));
    if (it == peers.end()) return false;
    out = it->second;
    return true;
}

std::vector<EQWPeerRecord> EQWNow::getPeers() const {
    std::vector<EQWPeerRecord> v;
    for (const auto& kv : peers) v.push_back(kv.second);
    return v;
}

void EQWNow::process() {
    if (!rxQueue) return;
    uint32_t now = millis();
    for (auto it = pendingReplies.begin(); it != pendingReplies.end();) {
        if (now - it->second.timestamp > pendingReplyTimeoutMs) {
            it = pendingReplies.erase(it);
        } else {
            ++it;
        }
    }
    QueuedMessage msg;
    while (xQueueReceive(rxQueue, &msg, 0) == pdTRUE) {
        if (msg.len < 7) continue;
        if (msg.data[0] != kEQWPrefix[0] ||
            msg.data[1] != kEQWPrefix[1] ||
            msg.data[2] != kEQWPrefix[2]) {
            continue;
        }

        uint8_t commandId = msg.data[3];
        uint8_t flag = msg.data[4];
        uint16_t requestId = (msg.data[5] << 8) | msg.data[6];
        const uint8_t* payload = msg.data + 7;
        size_t len = msg.len - 7;

        auto pending = pendingReplies.find(requestId);
        if (pending != pendingReplies.end()) {
            pending->second.cb(msg.mac, payload, len, flag, requestId);
            pendingReplies.erase(pending);
            continue;
        }

        auto it = callbacks.find(commandId);
        if (it != callbacks.end()) {
            it->second(msg.mac, payload, len, flag, requestId);
        } else if (commandId == 0x00) {
            handleSystemCommand(msg.mac, payload, len, flag, requestId);
        }
    }
}

bool EQWNow::sendSelfReport(const uint8_t* mac, uint16_t requestId) {
    const size_t maxLen = EQW_MAX_PAYLOAD_LEN;

    // calculate lengths with potential truncation
    size_t nameLen = strnlen(info.name, EQW_MAX_NAME_LEN);
    size_t cmdCount = registeredCommands.size();
    size_t payloadLen = 5 + 1 + nameLen + 1 + cmdCount;

    if (payloadLen > maxLen) {
        // try truncating the command list first
        size_t maxCmds = maxLen - (5 + 1 + nameLen + 1);
        if ((int)maxCmds < 0) maxCmds = 0;
        if (cmdCount > maxCmds) {
            cmdCount = maxCmds;
        }
        payloadLen = 5 + 1 + nameLen + 1 + cmdCount;
    }

    if (payloadLen > maxLen) {
        // truncate name if still too long
        size_t maxName = maxLen - (5 + 1 + 1 + cmdCount);
        if ((int)maxName < 0) maxName = 0;
        if (nameLen > maxName) {
            nameLen = maxName;
        }
        payloadLen = 5 + 1 + nameLen + 1 + cmdCount;
    }

    if (payloadLen > maxLen) {
        // even after truncation it's too big
        return false;
    }

    uint8_t payload[256];
    size_t idx = 0;
    payload[idx++] = info.deviceByteA;
    payload[idx++] = info.deviceByteB;
    payload[idx++] = info.versionMajor;
    payload[idx++] = info.versionMinor;
    payload[idx++] = info.versionPatch;
    payload[idx++] = static_cast<uint8_t>(nameLen);
    memcpy(payload + idx, info.name, nameLen);
    idx += nameLen;
    payload[idx++] = static_cast<uint8_t>(cmdCount);
    size_t count = 0;
    for (uint8_t cmd : registeredCommands) {
        if (count++ >= cmdCount) break;
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
    } else if (flag == 0x01 && payload && len >= 6) {
        EQWPeerRecord peer{};
        memcpy(peer.mac, mac, 6);
        size_t idx = 0;
        peer.info.deviceByteA = payload[idx++];
        peer.info.deviceByteB = payload[idx++];
        peer.info.versionMajor = payload[idx++];
        peer.info.versionMinor = payload[idx++];
        peer.info.versionPatch = payload[idx++];
        if (idx >= len) return;
        uint8_t nameLen = payload[idx++];
        if (nameLen > EQW_MAX_NAME_LEN || idx + nameLen > len) return;
        memcpy(peer.info.name, payload + idx, nameLen);
        peer.info.name[nameLen] = '\0';
        idx += nameLen;
        if (idx >= len) return;
        uint8_t cmdCount = payload[idx++];
        for (uint8_t i = 0; i < cmdCount && idx < len; ++i) {
            peer.supportedCommands.push_back(payload[idx++]);
        }
        storePeer(peer);
        if (selfReportCb) selfReportCb(peer, requestId);
    }
}

void EQWNow::onDataRecv(const uint8_t* mac, const uint8_t* data, int len) {
    if (!instance || !instance->rxQueue || len <= 0) return;
    QueuedMessage msg;
    memcpy(msg.mac, mac, 6);
    msg.len = len > 250 ? 250 : len;
    memcpy(msg.data, data, msg.len);
    xQueueSend(instance->rxQueue, &msg, 0);
}

void EQWNow::onDataSent(const uint8_t* mac, esp_now_send_status_t status) {
    // stub for future ack handling
}

