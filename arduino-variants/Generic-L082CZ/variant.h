/*
 * Copyright (c) 2017-2108 Thomas Roell.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal with the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimers.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimers in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of Thomas Roell, nor the names of its contributors
 *     may be used to endorse or promote products derived from this Software
 *     without specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * WITH THE SOFTWARE.
 */

#ifndef _VARIANT_GENERIC_L082CZ
#define _VARIANT_GENERIC_L082CZ

/*----------------------------------------------------------------------------
 *        Definitions
 *----------------------------------------------------------------------------*/

#define STM32L0_CONFIG_LSECLK             0
#define STM32L0_CONFIG_HSECLK             0
#define STM32L0_CONFIG_SYSOPT             0

/** Master clock frequency */
#define VARIANT_MCK			  F_CPU

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#ifdef __cplusplus
#include "Uart.h"
#endif // __cplusplus

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/*----------------------------------------------------------------------------
 *        Pins
 *----------------------------------------------------------------------------*/

// Number of pins defined in PinDescription array
#define PINS_COUNT           (38u)
#define NUM_DIGITAL_PINS     (38u)
#define NUM_ANALOG_INPUTS    (10u)
#define NUM_ANALOG_OUTPUTS   (2u)

/* Pin aliases: these give the GPIO port/bit for each pin as an
 * enum. These are optional, but recommended. They make it easier to
 * write code using low-level GPIO functionality. */
enum {
    PA0,PA1,PA2,PA3,PA4,PA5,PA6,PA7,PA8,PA9,PA10,PA11,PA12,PA13,PA14,PA15,
    PB0,PB1,PB2,PB3,PB4,PB5,PB6,PB7,PB8,PB9,PB10,PB11,PB12,PB13,PB14,PB15,
    PC0,PC1,PC2,PC13,PC14,PC15,
};

// Pins with fixed peripherals/connections

#define PIN_LED              PC15
#define LED_BUILTIN          PIN_LED

#define STM32L0_CONFIG_PIN_VBAT     PB1 // pin having voltage divider from battery
#define STM32L0_CONFIG_VBAT_SCALE   2.0 // voltage divider factor (Vbat=Vpin*fct)
#define STM32L0_CONFIG_CHANNEL_VBAT STM32L0_ADC_CHANNEL_9
#define STM32L0_CONFIG_VBAT_SMP     2000 // sampling time in nsec

/*
 * Analog pins
 */
#define PIN_A0               PA0 // confusing: "PIN_A0"=Analog 0, "PA0"=port A pin 0
#define PIN_A1               PA1
#define PIN_A2               PA2
#define PIN_A3               PA3
#define PIN_A4               PA4
#define PIN_A5               PA5
#define PIN_A6               PA6
#define PIN_A7               PA7
#define PIN_A8               PB0
#define PIN_A9               PB1
#define PIN_DAC0             PB3
#define PIN_DAC1             PB4

static const uint8_t A0  = PA0;
static const uint8_t A1  = PA1;
static const uint8_t A2  = PA2;
static const uint8_t A3  = PA3;
static const uint8_t A4  = PA4;
static const uint8_t A5  = PA5;
static const uint8_t A6  = PA6;
static const uint8_t A7  = PA7;
static const uint8_t A8  = PB0;
static const uint8_t A9  = PB1;

#define ADC_RESOLUTION		12

/*
 * Serial interfaces
 */

#define SERIAL_INTERFACES_COUNT 4  // omitting "lpuart1" for now

#define PIN_SERIAL_TX        PA9   // "usart1" (also on PB6/PB7)
#define PIN_SERIAL_RX        PA10

#define PIN_SERIAL1_TX       PA2   // "usart2" (also on PA14/PA13)
#define PIN_SERIAL1_RX       PA3
#define PIN_SERIAL1_CTS      PA0
#define PIN_SERIAL1_RTS      PA1

#define PIN_SERIAL2_TX       PA0   // "usart4"
#define PIN_SERIAL2_RX       PA3
#define PIN_SERIAL2_CTS      PB7
#define PIN_SERIAL2_RTS      PA15

#define PIN_SERIAL3_TX       PB3   // "usart5"
#define PIN_SERIAL3_RX       PB4
//#define PIN_SERIAL3_CTS      NONE
#define PIN_SERIAL3_RTS      PB5

/*
 * SPI Interfaces
 */
#define SPI_INTERFACES_COUNT 2

#define PIN_SPI_SCK          PA5
#define PIN_SPI_MISO         PA6
#define PIN_SPI_MOSI         PA7

static const uint8_t SS	  = PA4;
static const uint8_t MOSI = PIN_SPI_MOSI;
static const uint8_t MISO = PIN_SPI_MISO;
static const uint8_t SCK  = PIN_SPI_SCK;

#define PIN_SPI1_SCK          PB13
#define PIN_SPI1_MISO         PB14
#define PIN_SPI1_MOSI         PB15

/*
 * Wire Interfaces
 */
#define WIRE_INTERFACES_COUNT 3

#define PIN_WIRE_SCL         PB6  // "i2c1" (also on PA9/PA10)
#define PIN_WIRE_SDA         PB7

#define PIN_WIRE1_SCL        PB10 // "i2c2"
#define PIN_WIRE1_SDA        PB11

#define PIN_WIRE2_SCL        PC0  // "i2c3"
#define PIN_WIRE2_SDA        PC1

static const uint8_t SDA = PIN_WIRE_SDA;
static const uint8_t SCL = PIN_WIRE_SCL;


#define PWM_INSTANCE_COUNT    2

#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
 *        Arduino objects - C++ only
 *----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern Uart Serial;
extern Uart Serial1;
extern Uart Serial2;
extern Uart Serial3;
#endif

// These serial port names are intended to allow libraries and architecture-neutral
// sketches to automatically default to the correct port name for a particular type
// of use.  For example, a GPS module would normally connect to SERIAL_PORT_HARDWARE_OPEN,
// the first hardware serial port whose RX/TX pins are not dedicated to another use.
//
// SERIAL_PORT_MONITOR        Port which normally prints to the Arduino Serial Monitor
//
// SERIAL_PORT_USBVIRTUAL     Port which is USB virtual serial
//
// SERIAL_PORT_LINUXBRIDGE    Port which connects to a Linux system via Bridge library
//
// SERIAL_PORT_HARDWARE       Hardware serial port, physical RX & TX pins.
//
// SERIAL_PORT_HARDWARE_OPEN  Hardware serial ports which are open for use.  Their RX & TX
//                            pins are NOT connected to anything by default.
#define SERIAL_PORT_MONITOR         Serial
#define SERIAL_PORT_HARDWARE1       Serial
#define SERIAL_PORT_HARDWARE2       Serial1
#define SERIAL_PORT_HARDWARE3       Serial2
#define SERIAL_PORT_HARDWARE4       Serial3
#define SERIAL_PORT_HARDWARE_OPEN1  Serial1
#define SERIAL_PORT_HARDWARE_OPEN2  Serial2
#define SERIAL_PORT_HARDWARE_OPEN3  Serial3


#endif /*_VARIANT_GENERIC_L082CZ*/

