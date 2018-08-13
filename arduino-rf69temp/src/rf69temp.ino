// rftemp69 - Simple temperature sensor sending info via RF69 FSK module.
// Copyright 2018 Thorsten von Eicken

#include <Arduino.h>
#include <Wire.h>
#include "MCP9808.h"
//#include "STM32ADC.h"
#include "STM32L0.h"

#define LED LED_BUILTIN

//===== Configuration values and state variables

static constexpr uint16_t rate = 60; // standard interval between readings

static uint16_t rate_now = rate; // current rate
static uint16_t missed_acks = 0; // number of consecutive missed acks
static uint16_t blinks = 200; // number of times to blink LED for ack before going quiet
static uint16_t vMin = 0; // minimum battery voltage measured
static uint16_t vStart = 0; // battery voltage measured when coming out of sleep

//===== Hardware devices

MCP9808 tempSensor(Wire, 0x18);
//STM32ADC adc(ADC1);

//===== Utility functions

// batVoltage returns the battery voltage in mV
static int batVoltage () {
    return (int)(STM32L0.getVBAT()*1000+0.5);
}
#if 0
    adc.startConversion();
    int32_t raw = adc.read();
    adc.startConversion();
    raw += adc.read();
    int32_t vcc = (adc.measureVcc() + adc.measureVcc()) / 2;
    return (raw * vcc) / 4095;  // result in mV, with 1:2 divider
}
#endif

#if 0
static bool sendPkt(int data[], int n) {
    uint8_t pkt[64];
    pkt[0] = 0x80 + 1; // temp sensor packet type
    int len = encodeVarint(data, n, pkt+1, 63);
    len++; // account for pkt[0]
    rf.addInfo(pkt+len);
    rf.send(0x80, pkt, len); // hdr=0x80 -> send to node 0, request ACK
}
#endif

#if 0
// serial_putchar - char output function for printf
static int serial_putchar (char c, FILE *stream) {
   if( c == '\n' )
      Serial.write('\r');
   Serial.write(c) ;
   return 0 ;
}
static FILE fd1 = { 0 };   // FILE struct
static int uart_putchar(char c, FILE *stream);
#endif

//===== Setup

void setup() {
    // turn LED on before anything else
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);

    // init serial debug console
    Serial.begin(115200);
    delay(10);
    Serial.println("\n\n\r===== RF69temp starting =====\n");
    delay(100);

    // init I2C and temperature sensor
    Wire.begin();
    Wire.setClock(400000);
    if (!tempSensor.init()) {
        Serial.println("OOPS, can't init MCP9808!");
        while(1) ;
    }
    tempSensor.convert(); // flashing of LED is enough delay after enable

#if 0
    spi.init();
    if (!rf.init(62, 6, 912500)) {  // node 62, group 6, 912.5 MHz
        printf("OOPS, can't init radio!\r\n");
        while(1) ;
    }
#endif

    //pinMode(VBAT_PIN, INPUT);
    //adc.begin(VBAT_PIN);

    // blink 3x to signify end of setup, leave LED off
    for (int i=0; i<5; i++) { digitalWrite(LED, 1-digitalRead(LED)); delay(200); }
    Serial.println("Setup done.");
    delay(1000);

    //fdev_setup_stream(&uartout, uart_putchar, NULL, _FDEV_SETUP_WRITE);
    //stdout = &uartout;
}

//===== Loop

void loop() {
    vStart = batVoltage();
    if (vStart < vMin) vMin = vStart;
    int16_t uCTemp = (int16_t)(STM32L0.getTemperature() + 0.5);
    int32_t t = tempSensor.read();
    //Serial.println("Loop");
    printf("vBat: %d.%03dV->%d.%03dV uC:%dC mcp:%ld.%02ldC\r\n",
            vStart/1000, vStart%1000, vMin/1000, vMin%1000, uCTemp, t/100, t%100);

#if 0
    int data[8] = { t, 0, 0, 0, rf.txPow(), uCTemp, vStart, vMin };
    if (sendPkt(data, 8)) {
        if (blinks > 0) {
            led = 0;
            wait_ms(50);
            led = 1;
            blinks--;
        }
    } else {
        printf("no-ack\r\n");
    }
#endif
    vMin = batVoltage();

    delay(2000);
}
