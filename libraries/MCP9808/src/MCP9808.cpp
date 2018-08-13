// Driver for a MCP9808 temperature sensor
#if JEEH
#include <jee.h>
#elif ARDUINO
#include <Arduino.h>
#include <Wire.h>
#define I2C TwoWire
#endif

#include "MCP9808.h"

bool MCP9808::init () {
    //for (int i=1; i<8; i++)
    //    printf("reg %d = 0x%x\r\n", i, readReg(i));
    if (_regs.read(6) != 0x54 || _regs.read(7) != 0x400) return false;
    _regs.write(1, 0x100); // set shutdown mode
    _regs.write(8, 2);     // set resolution 2=0.125C, 3=0.0625C
    return true;
}

// convert puts the device into continuous conversion mode and return the number of
// milliseconds before the first conversion completes.
uint32_t MCP9808::convert() {
    _regs.write(1, 0); // set continuous conversion mode
    return 130;
}

// read returns the current temperature in 1/100th centigrade, assume a conversion is ready
int32_t MCP9808::read() {
    int32_t v = _regs.read(5);
    v = (v<<19)>>19; // sign-extend
    return (v*100 + 8) / 16;
}

void MCP9808::sleep() {
    _regs.write(1, 0x100);
}
