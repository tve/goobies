// Hello - blinks an LED once a second and prints a loop count.
#include <Arduino.h>
#include <stdio.h>

#define LED LED_BUILTIN

void setup() {
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);

    Serial.begin(115200);
    delay(10);
    Serial.println("Hello World!");
    //printf("Hello folks!\n");
}

uint8_t led=0;
int iter = 0;

void loop() {
    Serial.println("Hello!");
    //printf("Blink #%d\n", ++iter);
    led = 1-led;
    digitalWrite(LED, led);
    delay(500);
}
