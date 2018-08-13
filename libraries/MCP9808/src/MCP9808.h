// Driver for a MCP9808 temperature sensor
#ifndef _MCP9808_
#define _MCP9808_

#if JEEH

template< typename I2C, int addr =0x18 >
class MCP9808Regs {
    // read a 16-bit register
    uint16_t read (uint8_t r) {
        I2C::start(addr<<1);
        I2C::write(r);
        I2C::stop();
        I2C::start((addr<<1)|1);
        uint16_t v = I2C::read(false);
        return (v<<8) | I2C::read(true);
    }

    // write a 16-bit register
    void write (uint8_t r, uint16_t v) {
        I2C::start(addr<<1);
        I2C::write(r);
        if (r != 8) I2C::write(v>>8);
        I2C::write(v&0xff);
        I2C::stop();
    }
};

#elif ARDUINO

class MCP9808Regs {
    public:
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

class MCP9808 {
    public:
    MCP9808(MCP9808Regs &regs) : _regs(regs) { };
    bool init(); // set resolution and shutdown, return true iff device responded
    uint32_t convert(); // continuous mode, returns ms before first conversion
    int32_t read(); // temp in 1/100th centigrade
    void sleep(); // set sleep mode
    //private:
    MCP9808Regs &_regs;
};

#endif
