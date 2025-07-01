#include <EQWNow.h>
#include <EQWCommands.h>

EQWNow eqw;

void setup() {
    Serial.begin(115200);
    eqw.begin("Basic", 0x01, 0x02, 1, 0, 0);
    eqw.on("Power", [](const uint8_t* mac, const uint8_t* payload, size_t len, uint8_t flag, uint16_t req) {
        Serial.println("Power command received");
    });

    uint8_t broadcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    eqw.request(broadcast, eqwCommandIdForName("Battery"), 0x00, nullptr, 0,
                [](const uint8_t*, const uint8_t* payload, size_t len, uint8_t, uint16_t) {
                    if (len) Serial.printf("Battery level: %u\n", payload[0]);
                });
}

void loop() {
    eqw.process();
}
