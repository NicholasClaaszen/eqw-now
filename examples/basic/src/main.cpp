#include <EQWNow.h>

EQWNow eqw;

void setup() {
    Serial.begin(115200);
    eqw.begin("Basic", 0x01, 0x02, 1, 0, 0);
}

void loop() {
    eqw.process();
}
