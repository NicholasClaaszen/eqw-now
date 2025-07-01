#ifndef EQW_NOW_H
#define EQW_NOW_H

#include <Arduino.h>
#include <esp_now.h>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <array>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

const size_t EQW_MAX_NAME_LEN = 32;
const size_t EQW_MAX_PAYLOAD_LEN = 243; // max bytes allowed for payload in send()

struct EQWDeviceInfo {
    uint8_t deviceByteA = 0;
    uint8_t deviceByteB = 0;
    uint8_t versionMajor = 0;
    uint8_t versionMinor = 0;
    uint8_t versionPatch = 0;
    char name[EQW_MAX_NAME_LEN + 1] = {};
};

struct EQWPeerRecord {
    uint8_t mac[6];
    EQWDeviceInfo info;
    std::vector<uint8_t> supportedCommands;
};

class EQWNow {
public:
    using ReceiveCallback = std::function<void(const uint8_t*, const uint8_t*, size_t, uint8_t, uint16_t)>;
    using SelfReportCallback = std::function<void(const EQWPeerRecord&, uint16_t)>;

    EQWNow();

    bool begin(const char* name,
               uint8_t deviceA,
               uint8_t deviceB,
               uint8_t verMajor,
               uint8_t verMinor,
               uint8_t verPatch);

    void on(uint8_t commandId, ReceiveCallback cb);
    void on(const char* commandName, ReceiveCallback cb);

    bool send(const uint8_t* mac,
              uint8_t commandId,
              uint8_t flag,
              const uint8_t* payload,
              size_t len,
              uint16_t requestId = 0);

    uint16_t request(const uint8_t* mac,
                     uint8_t commandId,
                     uint8_t flag,
                     const uint8_t* payload,
                     size_t len,
                     ReceiveCallback replyCb);

    uint16_t queryDevices(const uint8_t* mac,
                          const std::vector<std::pair<uint8_t, uint8_t>>& deviceIds = {},
                          const std::vector<std::array<uint8_t, 6>>& macs = {});

    void onSelfReport(SelfReportCallback cb);
    void storePeer(const EQWPeerRecord& peer);
    bool getPeer(const uint8_t* mac, EQWPeerRecord& out) const;
    std::vector<EQWPeerRecord> getPeers() const;

    void setPendingReplyTimeout(uint32_t timeoutMs) { pendingReplyTimeoutMs = timeoutMs; }

    void process();

private:
    static void onDataRecv(const uint8_t* mac, const uint8_t* data, int len);
    static void onDataSent(const uint8_t* mac, esp_now_send_status_t status);

    bool sendSelfReport(const uint8_t* mac, uint16_t requestId);
    void handleSystemCommand(const uint8_t* mac,
                             const uint8_t* payload,
                             size_t len,
                             uint8_t flag,
                             uint16_t requestId);

    static EQWNow* instance;

    std::map<uint8_t, ReceiveCallback> callbacks;
    struct PendingReply {
        ReceiveCallback cb;
        uint32_t timestamp;
    };
    std::map<uint16_t, PendingReply> pendingReplies;
    uint32_t pendingReplyTimeoutMs = 10000;
    uint16_t nextRequestId = 1;

    std::set<uint8_t> registeredCommands;
    EQWDeviceInfo info;

    std::map<std::array<uint8_t, 6>, EQWPeerRecord> peers;
    SelfReportCallback selfReportCb = nullptr;

    QueueHandle_t rxQueue = nullptr;

    struct QueuedMessage {
        uint8_t mac[6];
        uint8_t data[250];
        uint8_t len;
    };
};

#endif // EQW_NOW_H
