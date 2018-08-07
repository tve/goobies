#include "utility/util_adc.h"

class STM32ADC {

public:

    // STM32ADC creates a device descriptor, it does not initialize anything, use begin() for
    // that purpose. Typical usage is `STM32ADC(ADC1)`.
    STM32ADC(ADC_TypeDef adc) : _adc(adc) {};

    // begin initializes the ADC device. It enables the clock, sets default conversion
    // parameters, and runs a calibration cycle (waiting for it to complete). It leaves the ADC
    // enabled in auto-off mode (if available).
    void begin();

    // recalibrate performs an ADC calibration cycle and should only be called if Vcc or temperature
    // conditions change significantly since the calibration done as part of begin().
    // It busy-waits until the calibration completes.
    void recalibrate() {

    // setSampleRate changes the sampling rate, the default is LL_ADC_SAMPLINGTIME_1CYCLE_5.
    // The set of possible sampling rates varies with uC type, see
    // system/Drivers/STM32L0xx_HAL_Driver/Inc/stm32l0xx_ll_adc.h or equivalent.
    // Note that sampleRate is a bit-field specific to the uC and not a true rate in Hz or such.
    void setSampleRate(uint32_t sampleRate);

    // setChannels configures which channels to convert using a channel scan.
    // For pin numbers, see setPins below ???
    void setChannels(uint8 *pins, uint8 length);

    // setPins configures the list of pins to convert using a channel scan.
    void setPins(uint8 *pins, uint8 length);

    // setTrigger determines how the start of a conversion is triggered. The default is to trigger
    // explicitly by software, i.e., LL_ADC_REG_TRIG_SOFTWARE. Other options are to trigger by a
    // timer or an external input, see stm32l0xx_ll_adc.h or equivalent.
    void setTrigger(uint32_t trigger);

    // attachInterrupt attaches a callback function to the ADC completion interrupt.
    void attachInterrupt(voidFuncPtr func, uint8 interrupt);

    // startConversion triggers the ADC to start converting.
    void startConversion();

    // startContinuous starts continuous ADC conversions.
    void startContinuous();

    // stopContinuous stops continuous ADC conversions.
    void resetContinuous();

    // read returns the DR register.
    uint32_t read();

    // Internal sources.

    // enableInternalReading configures the ADC to be able to read the internal Vcc and temperature.
    // This must be called before readVcc and readTemp.
    void enableInternalReading();

    // readVcc returns the internal measurement of Vcc in millivolts. It polls for the completion of
    // the ADC. Requires that enableInternalReading() be called beforehand.
    float readVcc();

    // readTemp returns the internal temperature measurement in degrees centigrade. It polls for the
    // completion of the ADC. Requires that enableInternalReading() be called beforehand.
    float readTemp();

    // DMA mode functions.

    // setDMA configures DMA with the ADC. It is independent of whether continuous mode or scan mode
    // are used. The callback() function is invoked when the DMA completes.
    void setDMA(uint16 *buf, uint16 bufLen, uint32 dmaFlags, voidFuncPtr callback);

    // setDMA configures DMA with dual ADC. It is independent of whether continuous mode or scan mode
    // are used. It assumes that the uC has two devices.
    void setDualDMA(uint32 *buf, uint16 bufLen, uint32 dmaFlags);

    // setScanMode enables scan mode use with DMA.
    void setScanMode();

    void attachDMAInterrupt(voidFuncPtr func);

    // Watchdog functions.

    // startAnalogWatchdog configures and enables the analog watchdog on a channel that is being
    // converted.
    void setAnalogWatchdog(uint8_t channel, uint32_t highLimit, uint32_t lowLimit);

    // checkAnalogWatchdog polls the status of the watchdog returning true if the watchdog fired.
    // It also resets the watchdog status.
    bool checkAnalogWatchdog();

    // attachAnalogWatchdogInterrupt attaches a callback function with the analog watchdog
    // interrupt.
    void attachAnalogWatchdogInterrupt(voidFuncPtr func);


private:
    uint32 _adc;
    voidFuncPtr _DMA_int;
    voidFuncPtr _ADC_int;
    voidFuncPtr _AWD_int;

};
