## RFM96/98 Spectrum Analyser

This code produces a real-time [432-434] or [914-916] MHz spectrum "waterfall" display:
The waterfall2 is an enhancement of the waterfall1 project and uses two sx1276-based modules
attached to a HyTiny (stm32f1).

![](waterfall.jpg)

Colours are mapped from black (min) => blue => yellow => red => white (max).  
The display scrolls smoothly and continuously from right to left.

The frequency step is approx 10kHz, resulting in a width of `240*10=2.4Mhz` across the display.
Each sweep takes approx 12ms and 4 sweeps are performed per pixel scan line.

### Hardware

Components used for this project:

* HyTiny - https://www.hotmcu.com/x-p-222.html
* 2.8" LCD - https://www.hotmcu.com/x-p-121.html
* RFM96 915Mhz - http://www.hoperf.com/rf\_transceiver/modules/RFM69CW.html
* RFM98 433Mhz - http://www.hoperf.com/rf\_transceiver/modules/RFM69CW.html

LCD connections, using the pins on the HyTiny's FPC-12 connector and flat cable:

* MOSI = PB5
* MISO = PB4
* SCLK = PB3
* NSEL = PB0
* DC   = PB6
* RST  = PB7
* LED  = PA15

RFM96 915Mhz connections, using the standard SPI1 pins:

* MOSI = PA7
* MISO = PA6
* SCLK = PA5
* NSEL = PA4
* DIO0 = PA3
* RST  = PA11

RFM98 433Mhz connections, using the standard SPI1 pins:

* MOSI = PA7
* MISO = PA6
* SCLK = PA5
* NSEL = PA8
* DIO0 = PA12
* RST  = PA11

Other pins can be assigned by adjusting the source code.

### Compiling and uploading

This project is set up for [PlatformIO](https://platformio.org) as toolchain:

* open this project in a supported IDE or do `cd examples/waterfall/`
* adjust `platformio.ini` as needed, in particular the upload port
* then build and upload, e.g. `pio run -t upload` from the command line

Found a bug? Please post on [GitHub](https://github.com/jeelabs/jeeh/issues).
