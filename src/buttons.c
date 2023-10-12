
#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint8_t pb_debounced_state = 0xFF;

void buttons_init(void) {
    // Enable pull-up resistors for PBs
    PORTA.PIN4CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN5CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN6CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN7CTRL = PORT_PULLUPEN_bm;

    // Configure timer for PB sampling
    cli();
    TCB0.CTRLB = TCB_CNTMODE_INT_gc; // Configure TCB0 in periodic interrupt mode
    TCB0.CCMP = 16667;               // Set interval for 5ms (16667 clocks @ 3.3 MHz)
    TCB0.INTCTRL = TCB_CAPT_bm;      // CAPT interrupt enable
    TCB0.CTRLA = TCB_ENABLE_bm;      // Enable
    sei();
}

ISR(TCB0_INT_vect) {
    static uint8_t count0 = 0;
    static uint8_t count1 = 0;

    // Sample pushbutton state
    uint8_t pb_state = PORTA.IN;
    uint8_t pb_changed = pb_state ^ pb_debounced_state;

    // Increment if PB state changed, reset otherwise
    count1 = (count1 ^ count0) & pb_changed;
    count0 = ~count0 & pb_changed;

    // Update pb_debounced_state if PB high for three samples
    pb_debounced_state ^= (count1 & count0); // | (pb_changed & pb_debounced_state); -> immediately on falling edge

    TCB0.INTFLAGS = TCB_CAPT_bm;    // Acknowledge interrupt
}