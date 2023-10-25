
#include <avr/io.h>

void pwm_init(void) {
    PORTB_DIRSET = PIN1_bm | PIN0_bm;                   // Enable PB1 (DISP EN) and PB0 (BUZZER) as outputs
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc;      // CLK_PER select, /1 prescaler (3.33 MHz)
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc | TCA_SINGLE_CMP1EN_bm | TCA_SINGLE_CMP0EN_bm; // Single-slope PWM mode, WO1 and WO0 enable
    TCA0.SINGLE.PER = 7161;                             // 465 Hz (A) is ~7161.29 clocks @ 3.33 MHz
    TCA0.SINGLE.CMP1 = 0;                               // off initially, Maximum brightness -> 100% duty cycle
    TCA0.SINGLE.CMP0 = 0;                               // BUZZER off initially -> 0% duty cycle || Max loudness = 50% duty cycle
    TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;          // Enable TCA0
}