
#include <avr/io.h>
#include <stdint.h>
#include "buzzer.h"
#include "spi.h"
#include "uart.h"

#define MIN_OCTAVE 1
#define MAX_OCTAVE 9

static volatile uint8_t octave = 4;

void buzzer_on(const uint8_t tone) { // Tutorial 12
    spi_init();
    if (tone == 0) {
        // spi_write(0b10111110);
        TCA0.SINGLE.PERBUF = 76472u >> (octave - 1);
        TCA0.SINGLE.CMP1BUF = TCA0.SINGLE.PERBUF;
        TCA0.SINGLE.CMP0BUF = (TCA0.SINGLE.PERBUF >> 1);
        // TCA0.SINGLE.CMP0BUF = 0;
    }
    else if (tone == 1) {
        // spi_write(0b11101011);
        TCA0.SINGLE.PERBUF = 90944u >> (octave - 1);
        TCA0.SINGLE.CMP1BUF = TCA0.SINGLE.PERBUF;
        TCA0.SINGLE.CMP0BUF = (TCA0.SINGLE.PERBUF >> 1);
        // TCA0.SINGLE.CMP0BUF = 0;
    }
    else if (tone == 2) {
        // spi_write(0b00111110);
        TCA0.SINGLE.PERBUF = 57288u >> (octave - 1);
        TCA0.SINGLE.CMP1BUF = TCA0.SINGLE.PERBUF;
        TCA0.SINGLE.CMP0BUF = (TCA0.SINGLE.PERBUF >> 1);
        // TCA0.SINGLE.CMP0BUF = 0;
    }
    else if (tone == 3) {
        // spi_write(0b01101011);
        TCA0.SINGLE.PERBUF = 152944u >> (octave - 1);
        TCA0.SINGLE.CMP1BUF = TCA0.SINGLE.PERBUF;
        TCA0.SINGLE.CMP0BUF = (TCA0.SINGLE.PERBUF >> 1);
        // TCA0.SINGLE.CMP0BUF = 0;
    }
}

void buzzer_off(void) {
    spi_init();
    TCA0.SINGLE.CMP1BUF = 0;
    TCA0.SINGLE.CMP0BUF = 0;
}

void buzzer_success_fail(void) {
    spi_init();
    TCA0.SINGLE.PERBUF = 7161;
    TCA0.SINGLE.CMP1BUF = 7161;
    TCA0.SINGLE.CMP0BUF = 0;
}

void inc_octave(void) {
    if (octave < MAX_OCTAVE) {
        octave++;
    }
}

void dec_octave(void) {
    if (octave > MIN_OCTAVE) {
        octave--;
    }
}

void reset_octave(void) {
    octave = 4;
}