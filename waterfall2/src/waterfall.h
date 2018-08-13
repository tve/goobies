// Use 2x RFM96 as spectrum waterfall display on an SPI-attached 320x240 LCD.
// See https://github.com/jeelabs/jeeh/tree/master/examples/waterfall2

//#include <jee.h>
//#include <jee/spi-rf96sa.h>
//#include <jee/text-font.h>
//#include "globals.h"

// shared globally even when there are multiple waterfall specializations

// waterfall_palette maps the range 0..255 is mapped as black -> blue -> yellow -> red -> white
// gleaned from the GQRX project by Moe Wheatley and Alexandru Csete (BSD, 2013)
// see https://github.com/csete/gqrx/blob/master/src/qtgui/plotter.cpp
static uint16_t waterfall_palette [256];

// wfConfigs holds a number of configurations for the waterfall display.
static struct { uint32_t ctr, step, bwConf; } wfConfigs[] = {
  { 912500000, 82, 1 },
  { 432500000, 82, 1 },
};


template <typename RF, typename LCD>
class waterfall {
    private: int setRgb (uint8_t r, uint8_t g, uint8_t b) {
        return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);  // rgb565
    }

    private: void initPalette () {
        uint16_t* p = waterfall_palette;
        for (int i = 0; i < 20; ++i)
            *p++ = setRgb(0, 0, 0);
        for (int i = 0; i < 50; ++i)
            *p++ = setRgb(0, 0, (140*i)/50);
        for (int i = 0; i < 30; ++i)
            *p++ = setRgb((60*i)/30, (125*i)/30, (115*i)/30+140);
        for (int i = 0; i < 50; ++i)
            *p++ = setRgb((195*i)/50+60, (130*i)/50+125, 255-(255*i)/50);
        for (int i = 0; i < 100; ++i)
            *p++ = setRgb(255, 255-(255*i)/100, 0);
        for (int i = 0; i < 6; ++i)
            *p++ = setRgb(255, (255*i)/5, (255*i)/5);
    };

    public: waterfall() {
        initPalette();
    }

    private: void sweepDisplay(LCD &lcd, int y, int count, uint8_t buf[]) {
        uint16_t pixelRow[count];
        for (int x = 0; x < count; ++x)
            pixelRow[x] = waterfall_palette[(uint8_t)(~buf[x])];
        if ((y & 0x1F) == 0) {
            for (int x=0; x<count; x+=count/4)
                pixelRow[x] = 0xFFFF; // white dot
        }
        lcd.bounds(count-1, y, y+1);         // write one line and set scroll
        lcd.pixels(0, y, pixelRow, count); // update display
    }

    private: void setFreq(RF& rf, uint32_t freq) {
        rf.writeReg(rf.REG_FRFMSB,   freq >> 16);
        rf.writeReg(rf.REG_FRFMSB+1, freq >> 8);
        rf.writeReg(rf.REG_FRFMSB+2, freq);
        //rf.writeReg(rf.REG_RXCONFIG, 0x28); // trigger RX restart not needed with FastHopOn
    }

    // sweepRadio performs one spectrum sweep from first by step for count steps. It stores the
    // samples in the provided buffer in the form -2*rssi, e.g., for -109dBm it stores 238.
    // The speed of the sweep is govered by usDelay, which is the number of microseconds to wait
    // between setting the frequency and reading the RSSI value.
    private: void sweepRadio(RF& rf, uint32_t first, uint32_t step, int count, uint8_t usDelay, uint8_t buf[]) {
        uint32_t freq = first;

        SysTick<72000000> delay;
        usDelay = usDelay >= 13 ? usDelay - 13 : 0;
        setFreq(rf, freq);
        delay.wait_us(100*usDelay);
        for (int x = 0; x < count; ++x) {
            if (usDelay > 0) delay.wait_us(usDelay);
            // set next freq before reading RSSI, gains a few us and doesn't seem to
            // affect the RSSI that is in the pipeline...
            freq += step;
            setFreq(rf, freq);
            // read RSSI
            *buf++ = rf.readReg(rf.REG_RSSIVALUE);
        }
        setFreq(rf, first); // step back takes longer, so start now
    }

    private: void dumpRow(int count, uint8_t buf[]) {
        for (int x = 0; x < count; ++x) {
            printf(" %d", buf[x]);
        }
        printf("\r\n");
        wait_ms(10000);
    }

    // packetDump prints the scanline if a threshold is exceeded
    private: void packetDump(int count, uint8_t buf[]) {
        // calculate some RSSI stats
        uint32_t sum=0, cnt=0;
        uint32_t max=0xff;
        for (int x = 0; x < count; ++x) {
            uint8_t rssi = buf[x];
            sum += (uint32_t)rssi;
            cnt++;
            if (rssi < max) max = rssi;
        }
        // dump if RSSI exceeds 80dBm
        if (max > 2*70) return;
        for (int x = 0; x < count; ++x) {
            printf(" %4d", -(int32_t)(buf[x]/2));
            if (x%20 == 19) printf("\r\n");
        }
        printf("\r\n");
        //wait_ms(500);
    }


    // statsPrint is not functional yet...
    private: void statsPrint(int count, uint8_t buf[], uint32_t f0, uint32_t fStep) {
        int32_t sum=0, cnt=0;
        int32_t max=0xff;
        int maxIx=0;
        int32_t wSum=0, wCnt=0;
        for (int x = 0; x < count; ++x) {
            int32_t rssi = (uint32_t)buf[x];
            sum += rssi;
            cnt++;
            if (rssi < 2*75) {
                wSum += x * (2*75-rssi);
                wCnt += 2*75-rssi;
            }
            if (rssi < max) {
                max = rssi;
                maxIx = x;
            }
        }
        int32_t avg = sum/cnt;
        if (max > 2*70) return;
        int32_t ctr = f0+fStep*(wSum/wCnt);

        printf("PKT: %ddBm @%dHz, center %dHz, avg %ddBm\r\n", -max/2, f0+maxIx*fStep, ctr, -avg/2);
    }

    // waterfall displays a spectrum analyzer waterfall, the which parameter selects from wfConfigs.
    public: void run(RF& rf, LCD& lcd, int which) {
        uint32_t center = wfConfigs[which].ctr;
        uint32_t middle = ((center<<2) / (32000000 >> 11)) << 6;

        //uint32_t step = bwConfigs[bwConfig].fdev;
        uint32_t step = wfConfigs[which].step;
        uint32_t first = middle - lcd.width/2 * step;
        uint32_t dF = lcd.width/2*step*61035/1000;
        //printf("middle: %d %x\r\n", middle, middle);

        initPalette();
        lcd.clear();
        for (int x=0; x<lcd.width; x+=lcd.width/4) {
            lcd.pixel(x, 12, 0xFFFF);
            lcd.pixel(x, 13, 0xFFFF);
            lcd.pixel(x, 14, 0xFFFF);
            lcd.pixel(x, 15, 0xFFFF);
            lcd_setXY(x, 1);
            lcd_printf("%dkHz", (center-dF+x*step*61035/1000+5000)/10000*10);
        }

        int bwConfig = wfConfigs[which].bwConf;
        initRadio(rf, bwConfig, center);
        rf.setMode(rf.MODE_RECEIVE);
        dumpRadioRegs(rf);
        wait_ms(10);

        while (true) {
            uint32_t start = ticks;

            printf("center %dkHz +/-%dkHz %d..%dkHz RxBW=%dHz step=%dHz\r\n",
                center / 1000, dF/1000, (center-dF)/1000, (center+dF)/1000,
                bwConfigs[bwConfig].fdev*61, step*61);

            static uint8_t rssiRow[lcd.width];

            for (int y = 16; y < lcd.height; ++y) {
                // sanity checks
                uint8_t mode = rf.readReg(rf.REG_OPMODE);
                if (mode != rf.MODE_RECEIVE) {
                    printf("OOPS: mode=%02x\r\n", mode);
                }
                uint8_t irq1 = rf.readReg(rf.REG_IRQFLAGS1);
                if (irq1 != 0xd0) {
                    printf("OOPS: irq1=%02x\r\n", irq1);
                }

                sweepRadio(rf, first, step, lcd.width, bwConfigs[bwConfig].delay, rssiRow);
                //dumpRow(lcd.width, rssiRow);
                sweepDisplay(lcd, y, lcd.width, rssiRow);
                //packetDump(lcd.width, rssiRow);
                //statsPrint(lcd.width, rssiRow, center-dF, step*61);

                if (nextMode()) { return; }
            }

            printf("screen=%dms sweep=%dms step=%dus\r\n",
                ticks - start, (ticks-start)/lcd.height, (ticks-start)*1000/lcd.height/lcd.width);
        }
    }

};
