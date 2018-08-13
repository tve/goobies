// Semtech SX1231 / HopeRF RF69 FSK packet radio driver

#include <Arduino.h>
#include <SPI.h>
#include "sx1231.h"

void SX1231::setMode (uint8_t newMode) {
    mode = newMode;
    writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0x7) | newMode);
    while ((readReg(REG_IRQFLAGS1) & IRQ1_MODEREADY) == 0)
        Yield();
}

void SX1231::setFrequency (uint32_t hz) {
    // accept any frequency scale as input, including KHz and MHz
    // multiply by 10 until freq >= 100 MHz (don't specify 0 as input!)
    while (hz < 100000000)
        hz *= 10;

    // Frequency steps are in units of (32,000,000 >> 19) = 61.03515625 Hz
    // use multiples of 64 to avoid multi-precision arithmetic, i.e. 3906.25 Hz
    // due to this, the lower 6 bits of the calculated factor will always be 0
    // this is still 4 ppm, i.e. well below the radio's 32 MHz crystal accuracy
    // 868.0 MHz = 0xD90000, 868.3 MHz = 0xD91300, 915.0 MHz = 0xE4C000
    uint32_t frf = (hz << 2) / (32000000L >> 11);
    writeReg(REG_FRFMSB, frf >> 10);
    writeReg(REG_FRFMSB+1, frf >> 2);
    writeReg(REG_FRFMSB+2, frf << 6);
}

void SX1231::configure (const uint8_t* p) {
    while (true) {
        uint8_t cmd = p[0];
        if (cmd == 0)
            break;
        writeReg(cmd, p[1]);
        p += 2;
    }
    mode = MODE_SLEEP;
}

// configRegs contains register-address, register-value pairs for initialization.
// Note that these are TvE's values, not JCW's, specifically, RxBW and Fdev have been optimized
// for rf69/rf96 and may not work with rf12's. Also, the sync bytes include one preamble byte to
// reduce the number of false sync word matches due to noise. These settings are compatible with the
// Go driver in https://github.com/tve/devices/tree/master/sx1231
static const uint8_t RF96configRegs [] = {
    0x01, 0x20, // FSK mode, high-freq regs, sleep mode
    0x01, 0x20, // FSK mode, high-freq regs, sleep mode
    0x02, 0x02, 0x03, 0x8A, // Bit rate: 49230bps
    0x04, 0x03, 0x05, 0x4E, // 51.5kHzFdev -> modulation index = 2.1
    0x09, 0xF0+11, // use PA_BOOST, start at 13dBm
    0x0A, 0x09, // no shaping, 40us TX rise/fall
    0x0B, 0x32, // Over-current protection @150mA
    0x0C, 0x20, // max LNA gain, no boost
    0x0D, 0x9E, // AFC on, AGC on
    0x0E, 0x02, // 8-sample rssi smoothing
    0x0F, 0x0A, // 10dB RSSI collision threshold
    0x10, 90*2, // RSSI threshold
    0x12, 0x52, // RxBW 83kHz
    0x13, 0x4A, // AfcBw 125kHz
    0x1A, 0x01, // clear AFC at start of RX
    0x1F, 0xCA, // 3 byte preamble detector, tolerate 10 chip errors (2.5 bits)
    0x20, 0x00, // No RX timeout if RSSI doesn't happen
    0x21, 0x00, // No RX timeout if no preamble
    0x22, 0x00, // No RX timeout if no sync
    0x23, 0x02, // delay 8 bits after RX end before restarting RX
    0x24, 0x07, // no clock out
    0x25, 0x00, 0x26, 0x05, // preamble 5 bytes
    0x27, 0x52, // auto-restart, 0xAA preamble, enable 3 byte sync
    0x28, 0xAA, // sync1: same as preamble, gets us additional check
    0x29, 0x2D, 0x2A, 0x2A, // sync2 (fixed), and sync3 (network group)
    0x30, 0xD0, // whitening, CRC on, no addr filt, CCITT CRC
    0x31, 0x40, // packet mode
    0x32, 255,  // max RX packet length
    0x35, 0x8F, // start TX when FIFO has 1 byte, FifoLevel intr when 15 bytes in FIFO
    0x44, 0x00, // no fast-hop
    0x4D, 0x07, // enable 20dBm tx power
    0
};

void SX1231::init (uint8_t id, uint8_t group, int freq) {
    myId = id;

    // b7 = group b7^b5^b3^b1, b6 = group b6^b4^b2^b0
    parity = group ^ (group << 4);
    parity = (parity ^ (parity << 2)) & 0xC0;

    do
        writeReg(REG_SYNCVALUE1, 0xAA);
    while (readReg(REG_SYNCVALUE1) != 0xAA);
    do
        writeReg(REG_SYNCVALUE1, 0x55);
    while (readReg(REG_SYNCVALUE1) != 0x55);

    configure(RF96configRegs);
    setFrequency(freq);

    writeReg(REG_SYNCVALUE3, group);

}

// txPower sets the transmit power to the requested level in dB. The driver assumes that the
// PA_BOOST pin is used in the radio module and allows adjustment from 2dBm to 20dBm.
void SX1231::txPower (uint8_t level) {
    if (level < 2) level = 2;
    if (level > 20) level = 20;
    setMode(MODE_STANDBY);
    if (level > 17) {
        writeReg(REG_PADAC, 0x87); // turn 20dBm mode on
        writeReg(REG_PACONFIG, 0xf0+level-5);
    } else {
        writeReg(REG_PACONFIG, 0xf0+level-2);
        writeReg(REG_PADAC, 0x84); // turn 20dBm mode off
    }
}

void SX1231::sleep () {
    setMode(MODE_SLEEP);
}

static uint8_t RF96lnaMap[] = { 0, 0, 6, 12, 24, 36, 48, 48 };

// receive initializes the radio for RX if it's not in RX mode, else it checks whether a
// packet is in the FIFO and pulls it out if it is. The returned value is -1 if there is no
// packet, else the length of the packet, which includes the destination address, source address,
// and payload bytes, but excludes the length byte itself. The fei, rssi, and lna values are also
// valid when a packet has been received but may change with the next call to receive().
int SX1231::receive (void* ptr, int len) {
    static uint8_t lastFlag;
    if (mode != MODE_RECEIVE) {
        setMode(MODE_RECEIVE);
        lastFlag = 0;
    } else {
        if ((readReg(REG_IRQFLAGS1) & IRQ1_SYNADDRMATCH) != lastFlag) {
            lastFlag ^= IRQ1_SYNADDRMATCH;
            if (lastFlag) { // flag just went from 0 to 1
                rssi = readReg(REG_RSSIVALUE);
                lna = RF96lnaMap[ (readReg(REG_LNAVALUE) >> 5) & 0x7 ];
                SPI::enable();
                SPI::transfer(REG_AFCMSB);
                int16_t f = SPI::transfer(0) << 8;
                f |= SPI::transfer(0);
                SPI::disable();
                afc = (int32_t)f * 61;
            }
        }

        if (readReg(REG_IRQFLAGS2) & IRQ2_PAYLOADREADY) {

            SPI::enable();
            SPI::transfer(REG_FIFO);
            uint8_t count = SPI::transfer(0); // first byte of packet is length
            for (uint8_t i=0; i<count; ++i) {
                uint8_t v = SPI::transfer(0);
                if (i < len)
                    ((uint8_t*) ptr)[i] = v;
            }
            SPI::disable();

            // only accept packets intended for us, or broadcasts
            // ... or any packet if we're the special catch-all node
            uint8_t dest = *(uint8_t*) ptr;
            if ((dest & 0xC0) == parity) {
                uint8_t destId = dest & 0x3F;
                if (destId == myId || destId == 0 || myId == 63)
                    return count;
            }
        }
    }
    return -1;
}

// send transmits the packet as specified by the header, which consists of the destination address
// in the lower 6 bits and bit 7 for ?? as well as bit 6 for ??.
// Note: the code is limited to len <62 because the FIFO is filled before initiating TX.
void SX1231::send (uint8_t header, const void* ptr, int len) {
    setMode(MODE_FSTX);

    SPI::enable();
    SPI::transfer(REG_FIFO | 0x80);
    SPI::transfer(len + 2);
    SPI::transfer((header & 0x3F) | parity);
    SPI::transfer((header & 0xC0) | myId);
    for (uint8_t i=0; i<len; ++i)
        SPI::transfer(((const uint8_t*) ptr)[i]);
    SPI::disable();

    setMode(MODE_TRANSMIT);
    while ((readReg(REG_IRQFLAGS2) & IRQ2_PACKETSENT) == 0)
        Yield();

    setMode(MODE_STANDBY);
}
