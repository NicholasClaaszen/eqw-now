#include <EQWNow.h>

EQWNow eqw;

void setup() {
    Serial.begin(115200);
    eqw.begin("Basic", 0x01, 0x02, 1, 0, 0);
    eqw.on("Power", [](const uint8_t* mac, const uint8_t* payload, size_t len, uint8_t flag, uint16_t req) {
        Serial.println("Power command received");
    });
}

void loop() {
    eqw.process();
}
