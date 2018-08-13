#include <Arduino.h>

#define LED PC15

void setup() {
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);

    // put your setup code here, to run once:
    Serial2.begin(115200);
    delay(10);
    Serial2.println("Serial enabled!");
}

uint8_t led=0;

void loop() {
    Serial2.println("Loop!");
    led = 1-led;
    digitalWrite(LED, led);
    delay(1000);
}
