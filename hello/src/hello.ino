// Hello - blinks an LED once a second and prints a loop count.
#include <Arduino.h>
#include <stdio.h>

#define LED PC15 // explicitly define PIN for LED, assumed active-low

void setup() {
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);

    Serial.begin(115200);
    delay(10);
    Serial.println("Hello World!");
    printf("Hello folks!\n");
}

uint8_t led=0;
int iter = 0;

void loop() {
    printf("Blink #%d\n", ++iter);
    led = 1-led;
    digitalWrite(LED, led);
    delay(500);
}
