// Use 2x RFM96 as spectrum waterfall display on an SPI-attached 320x240 LCD.
// See https://github.com/jeelabs/jeeh/tree/master/examples/waterfall2

#include <jee.h>
#include <jee/spi-rf96sa.h>
#include <jee/spi-rf96fsk.h>
#include <jee/text-font.h>

UartDev< PinA<9>, PinA<10> > console;

void printf(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt); veprintf(console.putc, fmt, ap); va_end(ap);
}

bool nextMode();
template <typename RF> void initRadio(RF& rf, uint8_t id, uint32_t freq);
template <typename RF> void dumpRadioRegs(RF& rf);

// Display

#if 0
SpiGpio< PinB<5>, PinB<4>, PinB<3>, PinB<0>, 1 > spiA;
#include <jee/spi-ili9325.h>
ILI9325< decltype(spiA) > lcd;
#else
#include <jee/spi-ili9341.h>
SpiGpio< PinB<5>, PinB<4>, PinB<3>, PinB<0>, 0 > spiA;
ILI9341< decltype(spiA), PinB<6> > lcd;
#endif

TextLcd< decltype(lcd) > text;
Font5x7< decltype(text) > lcd_text;

void lcd_printf(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt); veprintf(lcd_text.putc, fmt, ap); va_end(ap);
}

void lcd_setXY(int x, int y) { lcd_text.x = x; lcd_text.y = y; }

// Radios

SpiHw< PinA<7>, PinA<6>, PinA<5>, PinA<4> > spiB1;
RF96sa< decltype(spiB1) > rf915;
RF96fsk< decltype(spiB1) > fsk915;
SpiHw< PinA<7>, PinA<6>, PinA<5>, PinA<8> > spiB2;
RF96sa< decltype(spiB2) > rf433;

int group = 6;

#include "waterfall.h"
#include "monitor.h"

// Button

PinA<2> btn1; // button used to go to the next "mode"

template <typename T>
static void initBtn(T& btn) {
    btn.mode(Pinmode::in_pullup);
}

template <typename T>
static bool checkBtn(T& btn) {
    return btn.read() == 0;
}

template <typename T>
static void debounceBtn(T& btn) {
    while (true) {
      if (btn.read() != 0) {
        wait_ms(50);
        if (btn.read() != 0) return;
      }
    }
}

bool nextMode() {
    if (checkBtn(btn1)) { debounceBtn(btn1); return true; }
    return false;
}

// initLcd resets and inits the LCD, but does not clear it.
static void initLcd() {
    // rtp touch screen is on the same SPI bus, make sure it's disabled
    //PinB<2> rtpcs;
    //rtpcs = 1;
    //rtpcs.mode(Pinmode::out);

    // handle a couple of extra LCD pins that the driver doesn't deal with...
    // start with a reset pulse
    PinB<7> lcd_reset;
    lcd_reset = 0;
    lcd_reset.mode(Pinmode::out);
    // init SPI and end reset
    spiA.init();
    wait_ms(2);
    lcd_reset = 1;
    wait_ms(10); // data sheet says to wait 5ms
    // init the LCD controller
    lcd.init();
    // turn backlighting on
    PinA<15> lcd_light;
    lcd_light = 1;
    lcd_light.mode(Pinmode::out);
    //lcd.clear();
}

template <typename RF>
void initRadio(RF& rf, uint8_t id, uint32_t freq) {
    // issue reset pulse (note: this resets both radios!)
    PinA<11> rf_reset;
    rf_reset = 0;
    rf_reset.mode(Pinmode::out);
    wait_ms(1);
    // init both SPI and end reset
    spiB1.init();
    spiB2.init();
    rf_reset = 1;
    wait_ms(1);
    // init radio
    rf.init(id, group, freq);
    rf.setFrequency(freq);
    printf("Radio rev: %02x\r\n", rf.readReg(0x42));
}

// dump all radio registers
template <typename RF>
void dumpRadioRegs(RF& rf) {
    printf("   ");
    for (int i = 0; i < 16; ++i)
        printf("%3x", i);
    for (int i = 0; i < 0x80; i += 16) {
        printf("\r\n%02x:", i);
        for (int j = 0; j < 16; ++j)
            printf(" %02x", rf.readReg(i+j));
    }
    printf("\r\n");
}


int main () {
#if 1
    fullSpeedClock(); // 72Mhz
#else
    enableSysTick();  // 8Mhz
#endif
    printf("\r\n===== Waterfall 2 starting...\r\n");

    initBtn(btn1);

    // disable JTAG in AFIO-MAPR to release PB3, PB4, and PA15
    constexpr uint32_t afio = 0x40010000;
    MMIO32(afio + 0x04) |= 1 << 25; // disable JTAG, keep SWD enabled

    initLcd();
    //testPattern();
    lcd.freeze(16, 0);
    text.top = 16;

    waterfall<decltype(rf915), decltype(lcd)> wf1;
    waterfall<decltype(rf433), decltype(lcd)> wf2;
    pktmon<decltype(fsk915), decltype(lcd)> pkt1;

    int which = 2;
    while (true) {
      switch (which) {
      case 0:
        wf1.run(rf915, lcd, which);
        break;
      case 1:
        wf2.run(rf433, lcd, which);
        break;
      case 2:
        pkt1.run(fsk915, lcd, 912500);
        break;
      }
      which = (which+1)%3;
    }
}
