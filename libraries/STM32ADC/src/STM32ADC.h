class STM32ADC {

public:

    // STM32ADC creates a device descriptor, it does not initialize anything, use begin() for
    // that purpose. Typical usage is `STM32ADC(ADC1)`.
    STM32ADC(ADC_TypeDef *adc) : _adc(adc) {};

    // begin initializes the ADC device. It enables the clock, sets default conversion
    // parameters, runs a calibration cycle (waiting for it to complete) and sets the mux to the
    // specified pin. It leaves the ADC enabled in auto-off mode (if available). It returns true if
    // the pin can be converted by this ADC, the ADC is enabled regardless of the return value.
    bool begin(uint8_t pin);

    // end shuts down the ADC device and is primarily useful to save power.
    void end();

    // startConversion triggers the ADC to start converting.
    void startConversion();

    // read waits for a conversion to complete and returns the result.
    uint32_t read();

    // ready returns true if a conversion has completed.
    bool ready();

    // Advanced usage.

    // recalibrate performs an ADC calibration cycle and should only be called if Vcc or temperature
    // conditions change significantly since the calibration done as part of begin().
    // It busy-waits until the calibration completes.
    void recalibrate();

#if 0
    // setSampleRate changes the sampling rate, the default is LL_ADC_SAMPLINGTIME_1CYCLE_5.
    // The set of possible sampling rates varies with uC type, see
    // system/Drivers/STM32L0xx_HAL_Driver/Inc/stm32l0xx_ll_adc.h or equivalent.
    // Note that sampleRate is a bit-field specific to the uC and not a true rate in Hz or such.
    void setSampleRate(uint32_t sampleRate);

    // setPins configures the list of pins to convert. It configures a channel scan if more than one
    // pin is provided. It returns true if all pins can be convereted by this ADC.
    bool setPins(uint8_t *pins, uint8_t num);

    // setChannels configures which channels to convert using a channel scan.
    void setChannels(uint32_t channelBitmap);

    // setTrigger determines how the start of a conversion is triggered. The default is to trigger
    // explicitly by software, i.e., startConversion() / LL_ADC_REG_TRIG_SOFTWARE. Other options
    // are to trigger by a timer or an external input, see stm32l0xx_ll_adc.h or equivalent.
    void setTrigger(uint32_t trigger);

    // attachInterrupt attaches a callback function to the ADC completion interrupt.
    void attachInterrupt(voidFuncPtr func, uint8 interrupt);

    // startContinuous starts continuous ADC conversions.
    void startContinuous();

    // stopContinuous stops continuous ADC conversions.
    void resetContinuous();
#endif

    // Internal sources.

    // measureVcc performs an internal measurement of Vcc in millivolts. It saves and restores the
    // ADC state. Limitation: right now it requires the ADC to be in single conversion mode at the
    // outset.
    uint32_t measureVcc();

    // measureTemp performs an internal temperature measurement in degrees centigrade.
    // It saves and restores the ADC state. Limitation: right now it requires the ADC to be in
    // single conversion mode at the outset.
    int16_t measureTemp();

    // DMA mode functions.

#if 0
    // setDMA configures DMA with the ADC. It is independent of whether continuous mode or scan mode
    // are used. The callback() function is invoked when the DMA completes.
    void setDMA(uint16_t *buf, uint16_t bufLen, uint32_t dmaFlags, voidFuncPtr callback);

    // setDMA configures DMA with dual ADC. It is independent of whether continuous mode or scan mode
    // are used. It assumes that the uC has two devices.
    void setDualDMA(uint32_t *buf, uint16_t bufLen, uint32_t dmaFlags);

    // setScanMode enables scan mode use with DMA.
    void setScanMode();

    // attachDMAInterrupt attaches a callback function to the completion of a DMA operation.
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
#endif

private:
    ADC_TypeDef *_adc;
    //voidFuncPtr _DMA_int;
    //voidFuncPtr _ADC_int;
    //voidFuncPtr _AWD_int;

};
