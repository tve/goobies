#include <Arduino.h>
#include <stm32l0xx_ll_bus.h>
#include <stm32l0xx_ll_rcc.h>
#include <stm32l0xx_ll_adc.h>
#include "STM32ADC.h"

// begin initializes the ADC device. It enables the clock, sets default conversion
// parameters, runs a calibration cycle (waiting for it to complete) and sets the mux to the
// specified pin. It leaves the ADC enabled in auto-off mode (if available). It returns true if
// the pin can be converted by this ADC, the ADC is enabled regardless of the return value.
bool STM32ADC::begin(uint8_t pin) {
    PinName pn = digitalPinToPinName(pin);
    ADC_TypeDef *adc = (ADC_TypeDef *)pinmap_find_peripheral(pn, PinMap_ADC);
    if (adc == NULL || adc != _adc) return false;
    int chan = STM_PIN_CHANNEL(pinmap_find_function(pn, PinMap_ADC));

    // Not sure how to handle differences between STM32 series...
    //LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_ADC1); // STM32F1?
#ifdef STM32L0xx
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_ADC1); // STM32L0
    LL_APB2_GRP1_ForceReset(LL_APB2_GRP1_PERIPH_ADC1);
    LL_APB2_GRP1_ReleaseReset(LL_APB2_GRP1_PERIPH_ADC1);

    if (LL_RCC_HSI_IsReady()) {
        // HSI16 running, use async clock
        LL_ADC_SetClock(_adc, LL_ADC_CLOCK_ASYNC_DIV2); // Use div2 to avoid duty cycle issues
        LL_ADC_SetCommonFrequencyMode(__LL_ADC_COMMON_INSTANCE(), LL_ADC_CLOCK_FREQ_MODE_HIGH);
    } else {
        // HSI16 not running, use APB2 clock
        LL_ADC_SetClock(_adc, LL_ADC_CLOCK_SYNC_PCLK_DIV2); // Use div2 to avoid duty cycle issues
        // Assume that we're running at low frequency?
        LL_ADC_SetCommonFrequencyMode(__LL_ADC_COMMON_INSTANCE(), LL_ADC_CLOCK_FREQ_MODE_LOW);
    }

    LL_ADC_SetResolution(_adc, LL_ADC_RESOLUTION_12B);
    LL_ADC_SetDataAlignment(_adc, LL_ADC_DATA_ALIGN_RIGHT);
    LL_ADC_SetLowPowerMode(_adc, LL_ADC_LP_AUTOPOWEROFF);
    LL_ADC_SetSamplingTimeCommonChannels(_adc, 0); // 0 -> shortest sampling time

    LL_ADC_REG_SetTriggerSource(_adc, LL_ADC_REG_TRIG_SOFTWARE);
    LL_ADC_REG_SetContinuousMode(_adc, LL_ADC_REG_CONV_SINGLE);
    LL_ADC_REG_SetDMATransfer(_adc, LL_ADC_REG_DMA_TRANSFER_NONE);
    LL_ADC_REG_SetOverrun(_adc, LL_ADC_REG_OVR_DATA_OVERWRITTEN);

    LL_ADC_REG_SetSequencerDiscont(_adc, LL_ADC_REG_SEQ_DISCONT_DISABLE);
    LL_ADC_REG_SetSequencerChannels(_adc, 1<<chan);
#else
#error(unsupported processor)
#endif

    LL_ADC_StartCalibration(_adc);
    while (LL_ADC_IsCalibrationOnGoing(_adc)) ;
    LL_ADC_ClearFlag_EOCAL(_adc);

    LL_ADC_Enable(_adc);

#if 0
    printf("ADC initialized\n");
    delay(10);
    printf("ISR: %08lx\n", _adc->ISR);
    printf("CR:  %08lx\n", _adc->CR);
    printf("CFGR1: %08lx\n", _adc->CFGR1);
    printf("CFGR2: %08lx\n", _adc->CFGR2);
    printf("CHS: %08lx\n", _adc->CHSELR);
#endif

    return true;
}

// recalibrate performs an ADC calibration cycle and should only be called if Vcc or temperature
// conditions change significantly since the calibration done as part of begin().
// It busy-waits until the calibration completes.
void STM32ADC::recalibrate() {
    LL_ADC_Disable(_adc);
    while (LL_ADC_IsEnabled(_adc)) ; // Wait for disable to take effect
    LL_ADC_StartCalibration(_adc);
    while (LL_ADC_IsCalibrationOnGoing(_adc)) ;
    LL_ADC_Enable(_adc);
}

// end shuts down the ADC device and is primarily useful to save power.
void STM32ADC::end() {
    LL_ADC_Disable(_adc);
    LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_ADC1); // STM32L0
}

// startConversion triggers the ADC to start converting.
void STM32ADC::startConversion() {
    LL_ADC_REG_StartConversion(_adc);
}

// read waits for a conversion to complete and returns the result.
uint32_t STM32ADC::read() {
    while (LL_ADC_REG_IsConversionOngoing(_adc) && !LL_ADC_IsActiveFlag_EOC(_adc)) ;
    uint32_t v = LL_ADC_REG_ReadConversionData32(_adc);
    //_adc->ISR = _adc->ISR; // reset all flags
    return v;
}

// ready returns true if a conversion has completed.
bool STM32ADC::ready() {
    return !LL_ADC_REG_IsConversionOngoing(_adc);
}

// measureVcc performs an internal measurement of Vcc in millivolts. It saves and restores the
// ADC state. Limitation: right now it requires the ADC to be in single conversion mode at the
// outset.
uint32_t STM32ADC::measureVcc() {
    // FIXME: we're assuming the ADC is in a "simple" state of single conversion, etc. Should
    // basically save ADC state, re-init the ADC to vrefint, and then restore?
    LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(), LL_ADC_PATH_INTERNAL_VREFINT);
    uint32_t chans = LL_ADC_REG_GetSequencerChannels(_adc);
    uint32_t smpr = LL_ADC_GetSamplingTimeCommonChannels(_adc);

    LL_ADC_SetSamplingTimeCommonChannels(_adc, ADC_SMPR_SMP); // longest sampling time
    LL_ADC_REG_SetSequencerChannels(_adc, LL_ADC_CHANNEL_VREFINT);

    startConversion();
    uint32_t raw = read();
    uint32_t v = __LL_ADC_CALC_VREFANALOG_VOLTAGE(raw, LL_ADC_RESOLUTION_12B);

    // restore
    LL_ADC_SetSamplingTimeCommonChannels(_adc,smpr);
    LL_ADC_REG_SetSequencerChannels(_adc, chans);
    LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(), LL_ADC_PATH_INTERNAL_NONE);
    return v;
}

// measureTemp performs an internal temperature measurement in degrees centigrade.
// It saves and restores the ADC state. Limitation: right now it requires the ADC to be in
// single conversion mode at the outset.
int16_t STM32ADC::measureTemp() {
    // FIXME: we're assuming the ADC is in a "simple" state of single conversion, etc. Should
    // basically save ADC state, re-init the ADC to vrefint, and then restore?
    if (LL_ADC_REG_IsConversionOngoing(_adc)) {
        LL_ADC_REG_StopConversion(_adc);
        while (LL_ADC_REG_IsConversionOngoing(_adc)) ;
    }
    // start by enabling sensors
    LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(),
            LL_ADC_PATH_INTERNAL_VREFINT|LL_ADC_PATH_INTERNAL_TEMPSENSOR);
    uint32_t chans = LL_ADC_REG_GetSequencerChannels(_adc);
    uint32_t smpr = LL_ADC_GetSamplingTimeCommonChannels(_adc);

    LL_ADC_SetSamplingTimeCommonChannels(_adc, ADC_SMPR_SMP); // longest sampling time

    // start reading Vcc
    LL_ADC_REG_SetSequencerChannels(_adc, LL_ADC_CHANNEL_VREFINT);
    startConversion();
    uint32_t vcc = __LL_ADC_CALC_VREFANALOG_VOLTAGE(read(), LL_ADC_RESOLUTION_12B);

    // now read temperature
    LL_ADC_REG_SetSequencerChannels(_adc, LL_ADC_CHANNEL_TEMPSENSOR);
    startConversion();
    uint32_t raw = read();
    int32_t t = __LL_ADC_CALC_TEMPERATURE(vcc, raw, LL_ADC_RESOLUTION_12B);

    // restore
    LL_ADC_SetSamplingTimeCommonChannels(_adc,smpr);
    LL_ADC_REG_SetSequencerChannels(_adc, chans);
    LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(), LL_ADC_PATH_INTERNAL_NONE);
    return (int16_t)t;
}


#if 0



/*
Set the ADC Sampling Rate.
ADC_SMPR_1_5,               < 1.5 ADC cycles
ADC_SMPR_7_5,               < 7.5 ADC cycles
ADC_SMPR_13_5,              < 13.5 ADC cycles
ADC_SMPR_28_5,              < 28.5 ADC cycles
ADC_SMPR_41_5,              < 41.5 ADC cycles
ADC_SMPR_55_5,              < 55.5 ADC cycles
ADC_SMPR_71_5,              < 71.5 ADC cycles
ADC_SMPR_239_5,             < 239.5 ADC cycles
*/
void STM32ADC::setSampleRate(adc_smp_rate SampleRate){
    adc_set_sample_rate(_dev, SampleRate);
}

/*
Attach an interrupt to the ADC completion.
*/
void STM32ADC::attachInterrupt(voidFuncPtr func, uint8 interrupt){
    adc_attach_interrupt(_dev,interrupt, func);
}

/*
This will enable the internal readings. Vcc and Temperature
*/
void STM32ADC::enableInternalReading(){
    enable_internal_reading(_dev);
}

/*
This will read the Vcc and return something useful.
Polling is being used.
*/
float STM32ADC::readVcc(){
    unsigned int result = 0;
    float vcc = 0.0;
    result = adc_read(_dev, 17);

    vcc = (float)result * 1.1; //to be done later...
    return vcc;
}

/*
This will read the Temperature and return something useful.
Polling is being used.
*/
float STM32ADC::readTemp(){
    unsigned int result = 0;
    float temperature = 0.0;
    result = adc_read(_dev, 16);
    temperature = (float)((_V25-result)/_AverageSlope)+ 25.0;
    return temperature;
}

/*
This function will set the number of Pins to sample and which PINS to convert.
This uses the Maple Pin numbers and not the ADC channel numbers. Do not confuse.
*/
void STM32ADC::setPins(uint8 *pins, uint8 length){
    //convert pins to channels.
    uint8 channels[length];
    unsigned int records[3] = {0,0,0};
    unsigned char i = 0, j = 0;

    for (unsigned char i = 0; i < length; i++) { //convert the channels from pins to ch.
        channels[i] = PIN_MAP[pins[i]].adc_channel;
    }

    //run away protection
    if (length > 16) length = 16;

    //write the length
    records[2] |= (length - 1) << 20;

    //i goes through records, j goes through variables.
    for (i = 0, j = 0; i < length; i++) {//go through the channel list.
        if (i!=0 && i%6 == 0) j++;//next variable, please!!
        records[j] |= (channels[i] << ((i%6)*5));
    }
    //update the registers inside with the scan sequence.
    _dev->regs->SQR1 = records[2];
    _dev->regs->SQR2 = records[1];
    _dev->regs->SQR3 = records[0];
}

/*
This function will set the number of channels to convert
And which channels.
This is the ADC channels and not the Maple Pins!!! Important!!
Also, this will allow you to sample the AD and Vref channels.
*/
void STM32ADC::setChannels(uint8 *channels, uint8 length){
    adc_set_reg_seq_channel(_dev, channels, length);
}

/*
This function will set the trigger to start the conversion
Timer, SWStart, etc...
*/
void STM32ADC::setTrigger(adc_extsel_event trigger){
  adc_set_extsel(_dev, trigger);
}

/*
this function will set the continuous conversion bit.
*/
void STM32ADC::setContinuous(){
    _dev->regs->CR2 |= ADC_CR2_CONT;
};

/*
this function will reset the continuous bit.
*/
void STM32ADC::resetContinuous(){
    _dev->regs->CR2 &= ~ADC_CR2_CONT;
};

/*
This will be used to start conversions
*/
void STM32ADC::startConversion(){
    _dev->regs->CR2 |= ADC_CR2_SWSTART;
}

/*
This will set the Scan Mode on.
This will use DMA.
*/
void STM32ADC::setScanMode(){
    _dev->regs->CR1 |= ADC_CR1_SCAN;
}

void STM32ADC::calibrate() {
    adc_calibrate(_dev);
}

/*
This function is used to setup DMA with the ADC.
It will be independent of the mode used. It will either be used in continuous or scan mode
or even both... go figure. :)

The reason why this is a uint16 is that I am not ready for dual mode.
*/

void STM32ADC::setDMA(uint16 * Buf, uint16 BufLen, uint32 dmaFlags, voidFuncPtr func) {
//initialize DMA
    dma_init(DMA1);
//if there is an int handler to be called...
    if (func != NULL)
        dma_attach_interrupt(DMA1, DMA_CH1, func);
//enable ADC DMA transfer
    //adc_dma_enable(ADC1);
    _dev->regs->CR2 |= ADC_CR2_DMA;
//set it up...
    dma_setup_transfer(DMA1, DMA_CH1, &ADC1->regs->DR, DMA_SIZE_16BITS, Buf, DMA_SIZE_16BITS, dmaFlags);// Receive buffer DMA
//how many are we making??
    dma_set_num_transfers(DMA1, DMA_CH1, BufLen);
//enable dma.
    dma_enable(DMA1, DMA_CH1); // Enable the channel and start the transfer.
}

/*
This function is used to setup DMA with the ADC.
It will be independent of the mode used. It will either be used in continuous or scan mode
or even both...
This function is to be used with Dual ADC (the difference is to use 32bit buffers).
*/
void STM32ADC::setDualDMA(uint32 * Buf, uint16 BufLen, uint32 Flags){
    dma_init(DMA1);
    adc_dma_enable(_dev);
    dma_setup_transfer(DMA1, DMA_CH1, &_dev->regs->DR, DMA_SIZE_32BITS,//(DMA_MINC_MODE | DMA_CIRC_MODE)
                 Buf, DMA_SIZE_32BITS, Flags);// Receive buffer DMA
    dma_set_num_transfers(DMA1, DMA_CH1, BufLen);
    dma_enable(DMA1, DMA_CH1); // Enable the channel and start the transfer.
}

/*
This will set the Scan Mode on.
This will use DMA.
*/
void STM32ADC::attachDMAInterrupt(voidFuncPtr func){
    _DMA_int = func;
    dma_attach_interrupt(DMA1, DMA_CH1, func);
}

/*
This will set an Analog Watchdog on a channel.
It must be used with a channel that is being converted.
*/
void STM32ADC::setAnalogWatchdog(uint8 channel, uint32 HighLimit, uint32 LowLimit){
    set_awd_low_limit(_dev, LowLimit);
    set_awd_high_limit(_dev, HighLimit);
    set_awd_channel(_dev, channel);
}

/*
check analog watchdog
Poll the status on the watchdog. This will return and reset the bit.
*/
uint8 STM32ADC::getAnalogWatchdog(){
    return 1;
}

/*
Attach an interrupt to the Watchdog...
This can possibly be set together in one function and determine which peripheral
it relates to.
*/
void STM32ADC::attachAnalogWatchdogInterrupt(voidFuncPtr func){
    _AWD_int = func;

}
#endif
