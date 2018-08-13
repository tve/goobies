// ADC tester - Displays ADC value
#include <Arduino.h>
#include <stdio.h>
#include <STM32ADC.h>
//#include <stm32l0xx_ll_adc.h>
//#include <stm32l0xx_ll_rcc.h>

STM32ADC adc(ADC1);

void setup() {
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);

    Serial.begin(115200);
    delay(10);
    printf("===== ADC tester starting\n");

    if (!adc.begin(PB1)) printf("ADC.begin() failed!\n");
    pinMode(PB1, INPUT);
}

uint8_t led=0;

void loop() {
    adc.startConversion();
    uint32_t v = adc.read();
    uint32_t vcc = adc.measureVcc();
    int16_t temp = adc.measureTemp();
    printf("ADC: %lu, vcc: %lumV temp: %dC\n", v, vcc, temp);
    delay(1000);
}
