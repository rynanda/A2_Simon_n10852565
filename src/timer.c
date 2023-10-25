
#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint16_t count = 0;

void timer_init(void) {
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;
    TCB0.CCMP = 3333;                   // Set interval for 1ms (3333 clocks @ 3.3 MHz)
    TCB0.INTCTRL = TCB_CAPT_bm;         // CAPT interrupt enable
    TCB0.CTRLA = TCB_ENABLE_bm;         // Enable
}

ISR(TCB0_INT_vect) {
    count++;
    TCB0.INTFLAGS = TCB_CAPT_bm;        // Acknowledge interrupt
}