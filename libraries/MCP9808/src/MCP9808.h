// Driver for a MCP9808 temperature sensor
#ifndef _MCP9808_
#define _MCP9808_

#include <Arduino.h>
#include <Wire.h>

class MCP9808 {
    public:
    MCP9808(TwoWire& i2c = Wire, uint8_t addr = 0x18) : _i2c(i2c), _addr(addr) {};
    bool init();
    uint32_t convert(); // continuous mode, returns ms before first conversion
    int32_t read(); // temnp in 1/100th centigrade
    void sleep();
    //private:
    TwoWire &_i2c;
    uint8_t _addr;
    uint16_t readReg (uint8_t r);
    void writeReg (uint8_t r, uint16_t v);
};

#endif
