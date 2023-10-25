
// Include headers
#include <avr/io.h>
#include <stdint.h>
#include "tone.h"
#include "spi.h"
#include "uart.h"

// Define minimum and maximum octaves
#define MIN_OCTAVE 1
#define MAX_OCTAVE 9

// Define periods for each tone at maximum octave
#define E_HIGH 76472u
#define C_SHARP 90944u
#define A 57288u
#define E_LOW 152944u

// Default octave = 4
static volatile uint8_t octave = 4;

/* Turn on buzzer and display for specific tone - E(high), C#, A, E(low)
    For each tone, set CMP1 (display) at 100% duty cycle for max brightness
    and set CMP0 (buzzer) at 50% duty cycle for max loudness.*/
void tone_on(const uint8_t tone) {
    spi_init();
    if (tone == 0) {        // E(high)
        TCA0.SINGLE.PERBUF = E_HIGH >> (octave - 1);
        TCA0.SINGLE.CMP1BUF = TCA0.SINGLE.PERBUF;
        TCA0.SINGLE.CMP0BUF = (TCA0.SINGLE.PERBUF >> 1);
    }
    else if (tone == 1) {   // C#
        TCA0.SINGLE.PERBUF = C_SHARP >> (octave - 1);
        TCA0.SINGLE.CMP1BUF = TCA0.SINGLE.PERBUF;
        TCA0.SINGLE.CMP0BUF = (TCA0.SINGLE.PERBUF >> 1);
    }
    else if (tone == 2) {   // A
        TCA0.SINGLE.PERBUF = A >> (octave - 1);
        TCA0.SINGLE.CMP1BUF = TCA0.SINGLE.PERBUF;
        TCA0.SINGLE.CMP0BUF = (TCA0.SINGLE.PERBUF >> 1);
    }
    else if (tone == 3) {   // E(low)
        TCA0.SINGLE.PERBUF = E_LOW >> (octave - 1);
        TCA0.SINGLE.CMP1BUF = TCA0.SINGLE.PERBUF;
        TCA0.SINGLE.CMP0BUF = (TCA0.SINGLE.PERBUF >> 1);
    }
}

// Turn off buzzer and display
void tone_off(void) {
    spi_init();
    TCA0.SINGLE.CMP1BUF = 0;
    TCA0.SINGLE.CMP0BUF = 0;
}

// Only turn on display for showing success/fail patterns and user scores
void tone_success_fail(void) {
    spi_init();
    TCA0.SINGLE.PERBUF = 7161;
    TCA0.SINGLE.CMP1BUF = 7161;
    TCA0.SINGLE.CMP0BUF = 0;
}

// Increase octave by one if octave is under the max octave threshold (9)
void inc_octave(void) {
    if (octave < MAX_OCTAVE) {
        octave++;
    }
}

// Decrease octave by one if octave is above the min octave threshold (1)
void dec_octave(void) {
    if (octave > MIN_OCTAVE) {
        octave--;
    }
}

// Reset octave to default
void reset_octave(void) {
    octave = 4;
}