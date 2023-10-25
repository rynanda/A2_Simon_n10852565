
#include <avr/io.h>

void pwm_init(void) {
    PORTB_DIRSET = PIN1_bm | PIN0_bm;                   // Enable PB1 (DISP EN) and PB0 (BUZZER) as outputs
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc;      // CLK_PER select, /1 prescaler (3.33 MHz)
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc | TCA_SINGLE_CMP1EN_bm | TCA_SINGLE_CMP0EN_bm; // Single-slope PWM mode, WO1 and WO0 enable
    TCA0.SINGLE.PER = 7161;                             // 465 Hz (A) is ~7161.29 clocks @ 3.33 MHz
    TCA0.SINGLE.CMP1 = 0;                               // off initially, Maximum brightness -> 100% duty cycle
    TCA0.SINGLE.CMP0 = 0;                               // BUZZER off initially -> 0% duty cycle || Max loudness = 50% duty cycle
    TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;          // Enable TCA0

    /* 
        Modify with PERBUF, CMP1BUF, and CMP0BUF later based on next sequence/tone to play.
        E(high) = 465 ∗ 2^(−5/12) = 348.3563954 Hz ~ 348 Hz -> PERBUF = 9559, CMP1BUF = 9559, CMP0BUF = 9559/2
        C# = 465∗ 2^(−8/12) = 292.9316441 Hz ~ 293 Hz -> PERBUF = 11368, CMP1BUF = 11368, CMP0BUF = 11368/2
        A = 465 Hz -> PERBUF = 7161, CMP1BUF = 7161, CMP0BUF = 7161/2
        E(low) = 465∗ 2^(−17/12) = 174.1781977 Hz ~ 174 Hz -> PERBUF = 19118, CMP1BUF = 19118, CMP0BUF = 19118/2

        TONE1 | 465 * 2^(-5/12) = 348.3564 = 348    | Segments EF (LHS) = 0b10111110
        TONE2 | 465 * 2^(-8/12) = 292.9316 = 293    | Segments BC (LHS) = 0b11101011
        TONE3 | 465                                 | Segments EF (RHS) = 0b00111110
        TONE4 | 465 * 2^(-17/12) = 174.1782 = 174   | Segments BC (RHS) = 0b01101011
    */
}