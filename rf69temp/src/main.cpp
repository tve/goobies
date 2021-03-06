// Simple temperature sensor with RF communication to GW.

#include <jee.h>
#include <MCP9808.h>
#include <SX1231.h>
#include <jee/varint.h>

UartDev< PinA<9>, PinA<10> > console;

int printf(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt); veprintf(console.putc, fmt, ap); va_end(ap);
    return 0;
}

I2cBus< PinB<7>, PinB<6> > i2c;                     // standard I2C pins for SDA and SCL
MCP9808Jeeh< decltype(i2c) > mcp9808regs;           // std. chip on TvE's JZ4/JZ5
MCP9808 sensor(mcp9808regs);

#ifdef JNZ4
PinA<8> led;                                        // LED, active low
PinA<1> batPin;                                     // battery voltage divider
SpiGpio< PinB<5>, PinB<4>, PinB<3>, PinA<15> > spi; // default SPI1 pins
#else
PinC<15> led;                                       // LED, active low
PinB<1> batPin;                                     // battery voltage divider
SpiGpio< PinB<5>, PinB<4>, PinB<3>, PinC<14> > spi; // default SPI1 pins
#endif

SX1231Jeeh< decltype(spi) > sx1231regs;
SX1231 rf(sx1231regs);                              // RFM69 radio module
ADC<1> batVcc;                                      // ADC to measure battery voltage

// batVoltage returns the battery voltage in mV
static int batVoltage () {
    int adc = batVcc.read(batPin) + batVcc.read(batPin);
    int vcc = (batVcc.readVcc() + batVcc.readVcc()) / 2;
    return (adc * vcc) / 4095;  // result in mV, with 1:2 divider
}

static bool sendPkt(int32_t data[], int n) {
    uint8_t pkt[64];
    pkt[0] = 0x80 + 1; // temp sensor packet type
    int len = encodeVarint(data, n, pkt+1, 63);
    len++; // account for pkt[0]
    rf.addInfo(pkt+len); len+=2;
    printf("Sending %d bytes:", len);
    for (int i=0; i<len; i++) printf(" %02x", pkt[i]);
    printf("\n");
    rf.send(0x80, pkt, len); // hdr=0x80 -> send to node 0, request ACK
    //printf("Sent\n");

    while (1) {
        int l = rf.getAck(pkt, 64);
        if (l >= 0) return l > 0;
    }
}

void setup() {
    led.mode(Pinmode::out);
    led = 0;

    enableSysTick();
    uint32_t hz = defaultHz;

    console.init();
    console.baud(115200, hz);
    wait_ms(10);
    printf("\r\n===== RF69temp starting =====\r\n\n");

    i2c.init();
    detectI2c(i2c);

    if (!sensor.init()) {
        printf("OOPS, can't init MCP9808!\r\n");
        while(1) ;
    }
    sensor.convert(); // flashing of LED is enough delay after enable
    //wait_ms(sensor.convert()); // flashing of LED is enough delay

    spi.init();
    if (!rf.init(62, 6, 912500)) {  // node 62, group 6, 912.5 MHz
        printf("OOPS, can't init radio!\r\n");
        while(1) ;
    }
    rf.info();

    batPin.mode(Pinmode::in_analog);
    batVcc.init();

    for (int i=0; i<5; i++) { led = 1-led; wait_ms(200); }
    wait_ms(1000);
    printf("Setup done.\r\n");
}

static constexpr uint16_t rate = 60; // standard interval between readings
static uint16_t rate_now = rate; // current rate
static uint16_t missed_acks = 0; // number of consecitive missed acks
static uint16_t blinks = 200; // number of times to blink LED for ack before going quiet
static uint16_t vMin = 0; // minimum battery voltage measured
static uint16_t vStart = 0; // battery voltage measured when coming out of sleep

int main() {
    setup();

    while (true) {
        vStart = batVoltage();
        int16_t uCTemp = batVcc.readTemp();
        int32_t t = sensor.read();

        printf("vBat: %d.%03dV->%d.%03dV uC:%dC mcp:%d.%02dC\r\n",
                vStart/1000, vStart%1000, vMin/1000, vMin%1000, uCTemp, t/100, t%100);

        int32_t data[8] = { t, 0, 0, 0, rf.txpow, uCTemp, vStart, vMin };
        if (sendPkt(data, 8)) {
            if (blinks > 0) {
                led = 0;
                wait_ms(50);
                led = 1;
                blinks--;
            }
            printf("f=%d fei=%d pow=%d\n", rf.actFreq, rf.fei, rf.txpow);
        } else {
            printf("no-ack\r\n");
        }

        wait_ms(3000);
    }
}

// sample output:
//
