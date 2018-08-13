// Driver for a MCP9808 temperature sensor
#include "MCP9808.h"

bool MCP9808::init () {
    //for (int i=1; i<8; i++)
    //    printf("reg %d = 0x%x\r\n", i, readReg(i));
    if (readReg(6) != 0x54 || readReg(7) != 0x400) return false;
    writeReg(1, 0x100); // set shutdown mode
    writeReg(8, 2);     // set resolution 2=0.125C, 3=0.0625C
    return true;
}

// convert puts the device into continuous conversion mode and return the number of
// milliseconds before the first conversion completes.
uint32_t MCP9808::convert() {
    writeReg(1, 0); // set continuous conversion mode
    return 130;
}

// read returns the current temperature in 1/100th centigrade, assume a conversion is ready
int32_t MCP9808::read() {
    int32_t v = readReg(5);
    v = (v<<19)>>19; // sign-extend
    return (v*100 + 8) / 16;
}

void MCP9808::sleep() {
    writeReg(1, 0x100);
}

// read a 16-bit register
uint16_t MCP9808::readReg (uint8_t r) {
    uint8_t c = _i2c.requestFrom(_addr, 2, r, 1, true);
    if (c != 2) return 0; // may not be the best error value...
    int v = _i2c.read();
    return uint16_t((v<<8) | _i2c.read());
#if 0
    I2C::start(addr<<1);
    I2C::write(r);
    I2C::stop();
    I2C::start((addr<<1)|1);
    uint16_t v = I2C::read(false);
    return (v<<8) | I2C::read(true);
#endif
}

// write a 16-bit register
void MCP9808::writeReg (uint8_t r, uint16_t v) {
    _i2c.beginTransmission(_addr);
    _i2c.write(r);
    if (r != 8) _i2c.write(v>>8);
    _i2c.write(v&0xff);
    _i2c.endTransmission();
#if 0
    I2C::start(addr<<1);
    I2C::write(r);
    if (r != 8) I2C::write(v>>8);
    I2C::write(v&0xff);
    I2C::stop();
#endif
}
