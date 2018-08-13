// Use 2x RFM96 as spectrum waterfall display on an SPI-attached 320x240 LCD.
// See https://github.com/jeelabs/jeeh/tree/master/examples/waterfall2

#include <jee/varint.h>

// shared globally even when there are multiple waterfall specializations
static uint8_t pktbuf[128];
static int32_t ints[16];

template <typename R, typename L>
class pktmon {

    public: pktmon() { } ;
    public: void run(R& rf, L& lcd, uint32_t freq) {

        lcd.clear();
        lcd_setXY(1, 1);
        lcd_printf("nodes  a dBm kHz by ty: hex-id");
        lcd_setXY(0,16);

        initRadio(rf, 0, freq);
        dumpRadioRegs(rf);

        while (true) {
            if (nextMode()) return;

            int len = rf.receive(pktbuf, sizeof(pktbuf));
            //if (len > -1) printf("len=%d\r\n", len);
            if (len >= 3) {
                char ack = (pktbuf[1] & 0x80) ? 'A' : ' ';
                uint8_t typ = pktbuf[2];
                printf("%02d->%02d %c %4ddBm %3dkHz %2db t%02d:",
                        pktbuf[1]&0x3f, pktbuf[0]&0x3f, ack,
                        -rf.rssi>>1, rf.afc/1000, len, typ);
                lcd_printf("%02d->%02d %c%4d %3d %2d %02d: ",
                        pktbuf[1]&0x3f, pktbuf[0]&0x3f, ack,
                        -rf.rssi>>1, rf.afc/1000, len, typ);

                if (typ < 128) {
                    // varint encoded packet
                    int c = decodeVarint(pktbuf+3, len-3, ints, 16);
                    if (c <= 0) {
                        printf(" oops, more than 16 ints!\r\n");
                        lcd_printf(" oops, more than 16 ints!\n");
                        continue;
                    }
                    switch (typ) {
                    case 2: // temperature node
                        if (c != 9) break;
                        printf(" %08x %d.%02dC %dPa %2d.%02d%%rh %dlux %3ddBm %dC %d.%03dV..%d.%03dV\r\n",
                                ints[0], ints[1]/100, ints[1]%100, ints[2],
                                ints[3]/100, ints[3]%100,
                                ints[4], ints[5], ints[6],
                                ints[7]/1000, ints[7]%1000, ints[8]/1000, ints[8]%1000);
                        lcd_printf("%08x %d.%02dC\n", ints[0], ints[1]/100, ints[1]%100);
                        if (ints[2] != 0 || ints[3] != 0 || ints[4] != 0)
                            lcd_printf("    %dPa %2d.%02d%%rh %dlux\n",
                                    ints[2], ints[3]/100, ints[3]%100, ints[4]);
                        lcd_printf("    %3ddBm %dC %d.%03dV..%d.%03dV\n", ints[5], ints[6],
                                ints[7]/1000, ints[7]%1000, ints[8]/1000, ints[8]%1000);
                        continue;
                    }
                    // default: print ints
                    for (int i=0; i<c; i++) {
                        if (ints[i] < 0x100000) {
                            printf(" %d", ints[i]);
                            lcd_printf(" %d", ints[i]);
                        } else {
                            printf(" %x", ints[i]);
                            lcd_printf(" %x", ints[i]);
                        }
                    }
                } else {
                    // unknown packet encoding
                    for (int i=3; i<len; i++) {
                        printf(" %02x", pktbuf[i]);
                        lcd_printf(" %02x", pktbuf[i]);
                    }
                    break;
                }
                printf("\r\n");
                lcd_printf("\n");
            }
        }

    }
};
