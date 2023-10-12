
#ifndef INITIALISATION_H
#define INITIALISATION_H 1

#include <avr/io.h>

// Tutorial 12

/**
 * @brief Enable pull-up resistors for pushbuttons.
*/
static inline void buttons_init(void) {
    // Enable pull-up resistors for PBs
    PORTA.PIN4CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN5CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN6CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN7CTRL = PORT_PULLUPEN_bm;
}

/**
 * @brief Configure TCA0 for waveform output generation on PB0.
*/
void pwm_init(void) {
    PORTB_DIRSET = PIN0_bm;                             // Enable PB0 (BUZZER) as output
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc;      // CLK_PER select, /1 prescaler (3.33 MHz)
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc | TCA_SINGLE_CMP0EN_bm; // Single-slope PWM mode, WO0 enable
    TCA0.SINGLE.PER = 7161;                             // 465 Hz (A) is ~7161.29 clocks @ 3.33 MHz
    TCA0.SINGLE.CMP0 = 0;                               // BUZZER off initially -> 0% duty cycle || Max loudness = 50% duty cycle
    TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;          // Enable TCA0
}

/**
 * @brief Configure SPI for 7-seg display.
*/
static inline void spi_init(void) {
    // Configure pins
    // DISP LATCH (PA1)
    PORTA.OUTCLR = PIN1_bm;
    PORTA.DIRSET = PIN1_bm;
    PORTC.DIRSET = PIN0_bm | PIN2_bm;

    PORTMUX.SPIROUTEA = PORTMUX_SPI0_ALT1_gc;   // Select alternate pin configuration for SPI
    SPI0.CTRLB = SPI_SSD_bm;                    // Disable client select line
    SPI0.INTCTRL = SPI_IE_bm;                   // Enable interrupt
    SPI0.CTRLA = SPI_MASTER_bm | SPI_ENABLE_bm; // Enable SPI as master
}

/**
 * @brief Configure TCB1 for updating the 7-segment display and pushbutton debouncing.
*/
static inline void timer_init(void) {
    // Interrupt every 5ms
    TCB1.CCMP = 16667;
    TCB1.INTCTRL = TCB_CAPT_bm;
    TCB1.CTRLA = TCB_ENABLE_bm;
}

#endif