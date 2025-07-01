#ifndef EQW_NOW_H
#define EQW_NOW_H

#include <Arduino.h>
#include <esp_now.h>
#include <functional>
#include <map>
#include <set>
#include <vector>
#include <string>

const size_t EQW_MAX_NAME_LEN = 32;

struct EQWDeviceInfo {
    uint8_t deviceByteA = 0;
    uint8_t deviceByteB = 0;
    uint8_t versionMajor = 0;
    uint8_t versionMinor = 0;
    uint8_t versionPatch = 0;
    char name[EQW_MAX_NAME_LEN + 1] = {};
};

class EQWNow {
public:
    using ReceiveCallback = std::function<void(const uint8_t*, const uint8_t*, size_t, uint8_t, uint16_t)>;

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
    std::set<uint8_t> registeredCommands;
    EQWDeviceInfo info;
};

#endif // EQW_NOW_H
