// Driver for a MCP9808 temperature sensor
#ifndef _MCP9808_
#define _MCP9808_

struct MCP9808Regs {
    virtual uint16_t read (uint8_t r) const = 0; // read 16-bit register r
    virtual void write (uint8_t r, uint16_t v) const = 0; // write 16-bit register r
};

#if JEEH

template< typename I2C, int addr =0x18 >
struct MCP9808Jeeh : MCP9808Regs {
    MCP9808Jeeh() {};

    // read a 16-bit register
    uint16_t read (uint8_t r) const {
        I2C::start(addr<<1);
        I2C::write(r);
        I2C::stop();
        I2C::start((addr<<1)|1);
        uint16_t v = I2C::read(false);
        return (v<<8) | I2C::read(true);
    }

    // write a 16-bit register
    void write (uint8_t r, uint16_t v) const {
        I2C::start(addr<<1);
        I2C::write(r);
        if (r != 8) I2C::write(v>>8);
        I2C::write(v&0xff);
        I2C::stop();
    }
};

#elif ARDUINO

struct MCP9808Arduino : MCP9808Regs {
    MCP9808Regs(TwoWire &i2c, uint8_t addr = 0x18) : _i2c(i2c), _addr(addr) {};

    // read a 16-bit register
    uint16_t read (uint8_t r) {
        uint8_t c = _i2c.requestFrom(_addr, 2, r, 1, true);
        if (c != 2) return 0; // may not be the best error value...
        int v = _i2c.read();
        return uint16_t((v<<8) | _i2c.read());
    }

    // write a 16-bit register
    void write (uint8_t r, uint16_t v) {
        _i2c.beginTransmission(_addr);
        _i2c.write(r);
        if (r != 8) _i2c.write(v>>8);
        _i2c.write(v&0xff);
        _i2c.endTransmission();
    }

    //private:
    TwoWire &_i2c;
    uint8_t _addr;
};

#endif

struct MCP9808 {
    MCP9808(MCP9808Regs &regs) : _regs(regs) { };
    bool init(); // set resolution and shutdown, return true if device responded
    uint32_t convert(); // continuous mode, returns ms before first conversion
    int32_t read(); // temp in 1/100th centigrade
    void sleep(); // set sleep mode
    //private:
    MCP9808Regs &_regs;
};

#endif
