// Semtech SX1231 / HopeRF RF69 FSK packet radio driver
//
// Notes on configuring the sx1231 receiver:
//
// The datasheet is somewhat confused and confusing about what Fdev and RxBw really mean.
// Fdev is defined as the deviation between the center freq and the modulated freq, while
// conventionally the frequency deviation fdev is the difference between the 0 and 1 freq's,
// thus the conventional fdev is Fdev*2.
//
// Similarly the RxBw is specified as the single-sided bandwidth while conventionally the
// signal or channel bandwidths are defined using the total bandwidths.
//
// Given that the sx1231 is a zero-if receiver it is recommended to configure a modulation index
// greater than 1.2, e.g. best approx 2. Modulation index is defined as fdev/bit-rate. This
// means that Fdev should be approx equal to bps. [Martyn, or are you targeting a modulation
// index of 4?]
//
// The signal bandwidth (20dB roll-off) can be approximated by fdev + bit-rate. Since RxBw
// is specified as the single-sided bandwidth it needs to be at least (fdev+bit-rate)/2. Or,
// in sx1231 config terms, Fdev + bitrate/2. If AFC is used, in order to accommodate a crystal
// offset between Tx and Rx of Fdelta the AFC bandwidth should be approx fdev + bit-rate +
// 2*Fdelta.
//
// The default configuration used in this (TvE's) version of the driver sets the bit rate to
// 49.261kbps, the Fdev to 45khz (i.e. conventional fdev of 90khz), and single-sized RxBw to 100Mhz.
// This configuration has a modulation index of 1.8, which keeps most of the energy within the
// frequency deviation. Whether RxBW of 8khz or 100khz is better is debatable, it's a tradeoff
// between a tad more sensitivity at 83khz and more leeway to oscillator mis-match at 100khz.

#if JEEH
#include <jee.h>
#elif ARDUINO
#include <Arduino.h>
#include <SPI.h>
#endif
#include "SX1231.h"

// setMode switches the radio to a different operating mode. Note that this is not immediate and
// setMode does not wait for the switch to complete.
void SX1231::setMode (uint8_t newMode) {
    _mode = newMode;
    _regs.writeReg(REG_OPMODE, (newMode&0x7)<<2);
}

void SX1231::setFreq (uint32_t hz) {
    // accept any frequency scale as input, including KHz and MHz
    // multiply by 10 until freq >= 100 MHz (don't specify 0 as input!)
    while (hz < 100000000)
        hz *= 10;
    actFreq = hz;

#if 0
    // Frequency steps are in units of (32,000,000 >> 19) = 61.03515625 Hz
    // use multiples of 64 to avoid multi-precision arithmetic, i.e. 3906.25 Hz
    // due to this, the lower 6 bits of the calculated factor will always be 0
    // this is still 4 ppm, i.e. well below the radio's 32 MHz crystal accuracy
    // 868.0 MHz = 0xD90000, 868.3 MHz = 0xD91300, 915.0 MHz = 0xE4C000
    uint32_t frf = (hz << 2) / (32000000L >> 11);
    _regs.writeReg(REG_FRFMSB, frf >> 10);
    _regs.writeReg(REG_FRFMSB+1, frf >> 2);
    _regs.writeReg(REG_FRFMSB+2, frf << 6);
#else
    uint64_t frf = ((uint64_t)hz << 19) / (uint64_t)32000000;
    _regs.writeReg(REG_FRFMSB, frf >> 16);
    _regs.writeReg(REG_FRFMSB+1, frf >> 8);
    _regs.writeReg(REG_FRFMSB+2, frf);
#endif
}

void SX1231::configure (const uint8_t* p) {
    while (true) {
        uint8_t cmd = p[0];
        if (cmd == 0)
            break;
        _regs.writeReg(cmd, p[1]);
        p += 2;
    }
    _mode = MODE_SLEEP;
}

// configRegs contains register-address, register-value pairs for initialization.
// Note that these are TvE's values, not JCW's, specifically, RxBW and Fdev have been optimized
// for rf69/rf96 and may not work with rf12's. Also, the sync bytes include one preamble byte to
// reduce the number of false sync word matches due to noise. These settings are compatible with the
// Go driver in https://github.com/tve/devices/tree/master/sx1231
static const uint8_t SX1231configRegs [] = {
    0x01, 0x02, // standby mode, some regs don't program in sleep mode...
    0x02, 0x00, // packet mode, fsk
    0x03, 0x02, 0x04, 0x8A, // Bit rate: 49230bps
//  0x05, 0x02, 0x06, 0xE1, // 44kHzFdev -> modulation index = 1.8
    0x05, 0x03, 0x06, 0x4E, // 51.5kHzFdev -> modulation index = 2.1
//  0x05, 0x05, 0x06, 0xC3, // 90kHzFdev -> modulation index = 3.6
    0x0B, 0x00, // AFC low beta off
    0x19, 0x52, 0x1A, 0x4A, // RxBw 83khz, AFCBw 100khz
//  0x19, 0x4A, 0x1A, 0x42, // RxBw 100khz, AFCBw 125khz
    0x1E, 0x0C, // AFC auto-clear, auto-on
    0x26, 0x07, // disable clkout
    0x29, 0xB4, // RSSI thres -90dB
    0x2B, 0x40, // RSSI timeout after 128 bytes
    0x2D, 0x05, // Preamble 5 bytes
    0x2E, 0x90, // sync size 3 bytes
    0x2F, 0xAA, // sync1: 0xAA
    0x30, 0x2D, // sync2: 0x2D
    0x31, 0x2A, // sync3: network group
    0x37, 0xD0, // drop pkt if CRC fails // 0x37, 0xD8, // deliver even if CRC fails
    0x38, 0x42, // max 62 byte payload
    0x3C, 0x8F, // fifo thres
    0x3D, 0x10, // PacketConfig2, interpkt = 1, autorxrestart: 12=on/10=off
    0x6F, 0x30, // RegTestDAGC 20->continuous DAGC with low-beta offset, 30->w/out low-beta
    0x71, 0x09, // RegTestAfc   9->4392Hz low-beta offset
    0
};
static constexpr int32_t bw = 83000; // configured bandwidth
static constexpr int32_t br = 49230; // configured bit rate
static constexpr int32_t rssiTO = (int32_t)(10 * br / 16000) + 1; // RSSI timeout for ACK

bool SX1231::init (uint8_t id, uint8_t group, int freq) {
    myId = id;

    // b7 = group b7^b5^b3^b1, b6 = group b6^b4^b2^b0
    _parity = group ^ (group << 4);
    _parity = (_parity ^ (_parity << 2)) & 0xC0;

    int i = 0;
    do
        _regs.writeReg(REG_SYNCVALUE1, 0xAA);
    while (_regs.readReg(REG_SYNCVALUE1) != 0xAA && i++ < 10);
    if (i == 10) return false;
    i = 0;
    do
        _regs.writeReg(REG_SYNCVALUE1, 0x55);
    while (_regs.readReg(REG_SYNCVALUE1) != 0x55 && i++ < 10);
    if (i == 10) return false;

    configure(SX1231configRegs);
    setFreq(freq);

    _regs.writeReg(REG_SYNCVALUE3, group);
    return true;
}

void SX1231::info () {
    printf("SX1231:\n    00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n00:   ");
    for (int i=1; i<0x50; i++) {
        if (i % 16 == 0) printf("\n%02x:", i);
        printf(" %02x", _regs.readReg(i));
    }
    printf("\n");
}

// txPower sets the transmit power level in dB.
// This function is set-up for the sx1231's PA0, which is used in "up to +13dBm" modules.
void SX1231::txPower (int8_t dBm) {
    if (dBm > 13) dBm = 13;
    if (dBm < -18) dBm = -18;
    txpow = dBm;
    _regs.writeReg(REG_PALEVEL, 0x80 | (txpow+18));
}

// adjustPow changes the TX power based on the signal margin reported by the remote node in order to
// ensure that there is `target` of fade margin. It adjusts the power downwards by approx 25% of
// what is theoretically needed to hit the target and it adjusts the power upwards by the full amt
// needed.
// Note that his code has the min/max output power value hard-coded for PA0.
void SX1231::adjustPow (uint8_t margin, uint8_t target) {
    //int8_t txpow0 = txpow;
    if (margin > target+4 && txpow > -18) {
        txPower(txpow - (margin-target+2)/4);
    } else if (margin > target && txpow > -18) {
        txPower(txpow-1);
    } else if (margin < target && txpow < 13) {
        txPower(txpow + target-margin);
    }
    //if (txpow0 != txpow) printf("<TX {%d} %d->%ddB>", margin, txpow0, txpow);
}

// linkMargin calculates the dB of margin available given the SNR of a packet.
// The GW provides the SNR in the info trailer in its ACK packets. It calculates the SNR based on
// the RSSI of the received packet and the noise floor or the RSSI threshold it sets in its sx1231.
// From sec 3.5.3.2 "AGC Reference" the demodulator requires an SNR of 8dB + log10(2*RxBw).
int8_t SX1231::linkMargin (int8_t snr) {
    constexpr int8_t demod = 13; // 8dB + log10(2*45000)
    return snr - demod;
}

void SX1231::adjustFreq() {
    int32_t corr = fei/4; // apply 1/4 of error as correction
    if (corr > bw/4) corr = bw/4; // don't apply more than 1/4 of rx bandwidth
    if (corr < -bw/4) corr = -bw/4;
    setFreq(actFreq-corr); // apply correction
}

// sleep puts the sx1231 into the lowest power sleep mode.
void SX1231::sleep () {
    setMode(MODE_SLEEP);
}

// savePktMeta is an internal function to save the metadata for a received packet, such as RSSI,
// afc, etc.
void SX1231::savePktMeta() {
    static uint8_t lnaMap[] = { 0, 0, 6, 12, 24, 36, 48, 48 };
    rssi = _regs.readReg(REG_RSSIVALUE)/2;
    lna = lnaMap[ _regs.readReg(REG_LNAVALUE) & 0x7 ];
    // save freq error, use AFC correction value, which is more stable than current FEI
    int16_t f = _regs.readReg(REG_AFCMSB) << 8;
    f |= _regs.readReg(REG_AFCMSB+1);
    fei = (int32_t)f * -61; // AFC is correction, FEI is error, hence negation
}

int SX1231::savePkt(void* ptr, int len) {
    int16_t noise = _regs.readReg(REG_RSSIVALUE);
    int count = _regs.readPacket(ptr, len);
    noise += _regs.readReg(REG_RSSIVALUE);
    noise = -(noise>>2);
    margin = linkMargin(rssi>noise ? rssi-noise : 0);
#if 0
    printf("[PKT:%d@%d:", count, noise);
    for (int i=0; i<count; i++) printf(" %02x", ((uint8_t*)ptr)[i]);
    printf("]");
#endif
    return count;
}

// receive initializes the radio for RX if it's not in RX mode, else it checks whether a
// packet is in the FIFO and pulls it out if it is. The returned value is -1 if there is no
// packet, else the length of the packet, which includes the destination address, source address,
// and payload bytes, but excludes the length byte itself. The fei, rssi, and lna values are also
// valid when a packet has been received but may change with the next call to receive().
int SX1231::receive (void* ptr, int len) {
    static uint8_t lastFlag;
    if (_mode != MODE_RECEIVE) {
        setMode(MODE_RECEIVE);
        lastFlag = 0;
        return -1;
    }

    if ((_regs.readReg(REG_IRQFLAGS1) & IRQ1_SYNADDRMATCH) != lastFlag) {
        lastFlag ^= IRQ1_SYNADDRMATCH;
        if (lastFlag) savePktMeta(); // flag just went from 0 to 1
    }

    if (_regs.readReg(REG_IRQFLAGS2) & IRQ2_PAYLOADREADY) {
        int count = savePkt(ptr, len);

        // only accept packets intended for us, or broadcasts
        // ... or any packet if we're the special catch-all node
        uint8_t dest = *(uint8_t*) ptr;
        if ((dest & 0xC0) == _parity) {
            uint8_t destId = dest & 0x3F;
            if (destId == myId || destId == 0 || myId == 63)
                return count;
        }
    }
    return -1;
}

// readAck assumes that a packet is ready in the FIFO, reads it and processes the FEI and SNR
// info it carries in the first two bytes to adjust TX power and frequency. It copies the packet
// to the provided buffer and returns its length.
int SX1231::readAck(void* ptr, int len) {
    int l = savePkt(ptr, len);
    if (l < 2) return 0;
    uint8_t *buf = (uint8_t*)ptr; // get a pointer we can dereference
    if ((buf[0] & 0xC0)!= _parity) return 0; // bad group parity
    if ((buf[0] & 0x3F) != myId) return 0; // not for us
    if (l == 2) return 2; // old-style ACK
    if ((buf[1] & 0x80) != 0) return 0; // not an ACK packet
    // it's an ACK from GW (should we check source addr?)
    adjustFreq(); // adjust based on what we measured, not what GW says...
#if ADJPOW
    if ((buf[2] & 0x80) != 0 && l > 4) { // there is an info trailer
        adjustPow(buf[l-2]);
    }
#endif
    return l;
}

// getAck briefly turns on the receiver to get an ACK to a just-transmitted packet. It returns the
// number of bytes received, or -1 if more waiting is needed, or 0 if the ack wait timed out.
// If an ack is receivced and it carries FEI and SNR info then adjustPowFreq is called.
int SX1231::getAck(void* ptr, int len) {
    static uint8_t lastFlag;
    uint8_t mode = (_regs.readReg(REG_OPMODE)>>2) & 0x7;

    //static uint8_t lm = -1;
    //if (mode != lm)
    //{ printf("{%d|%02x|%02x}", mode, _regs.readReg(REG_IRQFLAGS1), _regs.readReg(REG_IRQFLAGS2)); lm=mode; }

    if (mode == MODE_FS) return -1; // need to wait

    if (mode == MODE_RECEIVE) {
        uint8_t irqFlags1 = _regs.readReg(REG_IRQFLAGS1);
        //printf("{%d,%02x}", mode, irqFlags);

        if ((irqFlags1 & IRQ1_SYNADDRMATCH) != lastFlag) {
            lastFlag ^= IRQ1_SYNADDRMATCH;
            if (lastFlag) savePktMeta(); // flag just went from 0 to 1
        }
        if ((irqFlags1 & IRQ1_TIMEOUT) != 0) {
            // ack wait timed out :-(
#if ADJPOW
            if (txpow < 13) txPower(txpow+1);
#endif
            setMode(MODE_STANDBY);
            return 0;
        }
    }

    uint8_t irqFlags2 = _regs.readReg(REG_IRQFLAGS2);
    if (mode == MODE_TRANSMIT && (irqFlags2 & IRQ2_PACKETSENT) != 0) {
        // just finished TX, need to switch to RX
        _regs.writeReg(REG_TIMEOUT1, rssiTO);   // timeout after rx enable 'til rssi thres
        _regs.writeReg(REG_TIMEOUT2, len/2+10); // timeout after rssi thres 'til packetready
        setMode(MODE_RECEIVE);
        lastFlag = 0;
        return -1;
    }

    if ((irqFlags2 & IRQ2_PAYLOADREADY) != 0) {
        // got ack!
        int l = readAck(ptr, len);
        setMode(MODE_STANDBY);
        return l;
    }

    return -1;
}

// Add 2 bytes of info at ptr[0] and ptr[1] to the outgoing packet to signal margin and FEI to the
// other party. Note that bit 7 in packet type byte needs to be set too!
void SX1231::addInfo(uint8_t *ptr) {
    int8_t m = margin;
    if (m > 63) m = 63;
    if (m < 0) m = 0;
    ptr[0] = m;
    ptr[1] = (fei+64) >> 7;
}

// send transmits the packet as specified by the header, which consists of the destination address
// in the lower 6 bits and bit 7 for ?? as well as bit 6 for ??.
// Note: the code is limited to len <62 because the FIFO is filled before initiating TX.
void SX1231::send (uint8_t header, const void* ptr, int len) {
    setMode(MODE_FS);
    //printf("{TX:%02x %02x}\n", (header & 0x3F) | _parity, (header & 0xC0) | myId);
    _regs.writePacket((header & 0x3F) | _parity, (header & 0xC0) | myId, ptr, len);
    setMode(MODE_TRANSMIT);
}
