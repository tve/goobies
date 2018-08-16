// Semtech SX1231 / HopeRF RF69 FSK packet radio driver
//
// JeeLabs packet format:
//  0: packet length N = 2..65
//  1: group parity (b7..6), dest id (b5..0)
//  2: flags (b7:ACk-req, b6:unused), src id (b5..0)
//  3..(N-1): payload data (max 63 bytes)
//  N..(N+1): 16-bit crc

struct SX1231Regs {
    // read an 8-bit register
    virtual uint8_t readReg (uint8_t addr) const = 0;
    // write an 8-bit register
    virtual void writeReg (uint8_t addr, uint8_t val) const = 0;
    // read a packet, return length
    virtual int readPacket (void* ptr, int len) const = 0;
    // write a packet with two header bytes, len is just for data
    virtual void writePacket (uint8_t hdr1, uint8_t hdr2, const void* ptr, int len) const = 0;
};

#if JEEH

template< typename SPI >
struct SX1231Jeeh : SX1231Regs {
    // read an 8-bit register
    uint8_t readReg (uint8_t addr) const { return rwReg(addr, 0); }
    // write an 8-bit register
    void writeReg (uint8_t addr, uint8_t val) const { rwReg(addr | 0x80, val); }

    // private:
    // write and read a byte
    uint8_t rwReg (uint8_t cmd, uint8_t val) const {
        SPI::enable();
        SPI::transfer(cmd);
        uint8_t r = SPI::transfer(val);
        SPI::disable();
        return r;
    }

    static constexpr int REG_FIFO = 0x00;

    // write a packet with two header bytes, len is just for data
    void writePacket (uint8_t hdr1, uint8_t hdr2, const void* ptr, int len) const {
        SPI::enable();
        SPI::transfer(REG_FIFO | 0x80);
        SPI::transfer(len + 2);
        SPI::transfer(hdr1);
        SPI::transfer(hdr2);
        for (uint8_t i=0; i<len; ++i)
            SPI::transfer(((const uint8_t*) ptr)[i]);
        SPI::disable();
    }

    // read a packet, return length
    int readPacket (void* ptr, int len) const {
            SPI::enable();
            SPI::transfer(REG_FIFO);
            int count = SPI::transfer(0); // first byte of packet is length
            for (int i=0; i<count; ++i) {
                uint8_t v = SPI::transfer(0);
                if (i < len) ((uint8_t*) ptr)[i] = v;
            }
            SPI::disable();
            return count;
    }
};

#elif ARDUINO

#error("not implemented yet")

#endif

struct SX1231 {
    SX1231(SX1231Regs &regs) : _regs(regs) {}

    bool init (uint8_t id, uint8_t group, int freq);

    void txPower (int8_t dBm); // set power: -18dBm..13dBm
    void adjustPow (uint8_t margin, uint8_t target);
    void adjustFreq();

    int receive (void* ptr, int len);
    void send (uint8_t header, const void* ptr, int len);
    int getAck (void* ptr, int len); // turn RX on for short period to RX ACK
    int readAck (void* ptr, int len); // read ACK from FIFO, return length
    void addInfo(uint8_t *ptr); // add info about last RX to outgoing packet
    void sleep (); // put the radio to sleep to save power
    int8_t linkMargin (int8_t snr);
    void info();

    // current config
    uint8_t myId;
    uint32_t actFreq; // actual frequency
    int8_t txpow;     // current tx power in dB

    // info about last packet received
    int32_t fei;    // freq error of last pkt received
    int16_t rssi;   // RSSI of last packet received
    int8_t  snr;    // SNR in dB of last packet received
    int8_t  margin; // signal margin in dB of last packet received, based on SNR
    uint8_t lna;    // LNA attenuation in dB

    //private: // commented out 'cause it's a PITA when one needs something special

    enum {
        REG_FIFO          = 0x00,
        REG_OPMODE        = 0x01,
        REG_FRFMSB        = 0x07,
        REG_PALEVEL       = 0x11,
        REG_LNAVALUE      = 0x18,
        REG_AFCMSB        = 0x1F,
        REG_FEIMSB        = 0x21,
        REG_RSSIVALUE     = 0x24,
        REG_IRQFLAGS1     = 0x27,
        REG_IRQFLAGS2     = 0x28,
        REG_TIMEOUT1      = 0x2A,
        REG_TIMEOUT2      = 0x2B,
        REG_SYNCVALUE1    = 0x2F,
        REG_SYNCVALUE3    = 0x31,
        //REG_NODEADDR      = 0x33,
        //REG_BCASTADDR     = 0x34,
        REG_FIFOTHRESH    = 0x3C,
        REG_PKTCONFIG2    = 0x3D,
        REG_AESKEYMSB     = 0x3E,

        MODE_SLEEP        = 0,
        MODE_STANDBY      = 1,
        MODE_FS           = 2,
        MODE_TRANSMIT     = 3,
        MODE_RECEIVE      = 4,

        //START_TX          = 0xC2,
        //STOP_TX           = 0x42,

        //RCCALSTART        = 0x80,
        IRQ1_MODEREADY    = 1<<7,
        IRQ1_RXREADY      = 1<<6,
        IRQ1_TIMEOUT      = 1<<2,
        IRQ1_SYNADDRMATCH = 1<<0,

        IRQ2_FIFONOTEMPTY = 1<<6,
        IRQ2_PACKETSENT   = 1<<3,
        IRQ2_PAYLOADREADY = 1<<2,
    };

    void setMode (uint8_t newMode);
    void configure (const uint8_t* p);
    void setFreq (uint32_t freq);
    void savePktMeta();
    int savePkt(void *ptr, int len);

    uint8_t _parity;
    uint8_t _mode;
    SX1231Regs &_regs;
};
