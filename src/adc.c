
#include <avr/io.h>

void adc_init(void) { // Tutorial 8 // For playback delay -> scaling to between 0.25 and 2 seconds, increase delay = clockwise, vice versa
    ADC0.CTRLA = ADC_ENABLE_bm;                                         // Enable ADC
    ADC0.CTRLB = ADC_PRESC_DIV2_gc;                                     // /2 clock prescaler
    ADC0.CTRLC = (4 << ADC_TIMEBASE_gp) | ADC_REFSEL_VDD_gc;
    ADC0.CTRLE = 64;                                                    // Sample duration 64
    ADC0.CTRLF = ADC_FREERUN_bm;                                        // Free running mode
    ADC0.MUXPOS = ADC_MUXPOS_AIN2_gc;                                   // Select AIN2 (potentiometer R1)
    ADC0.COMMAND = ADC_MODE_SINGLE_8BIT_gc | ADC_START_IMMEDIATE_gc;    // 8-bit single conversion mode, start sampling immediately
}