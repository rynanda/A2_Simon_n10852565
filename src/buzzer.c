
#include <avr/io.h>
#include <stdint.h>
#include "buzzer.h"
#include "spi.h"

// Values used to write to the TCA0.SINGLE.PERBUF register
#define TONE1_Ehigh_PER 9559
#define TONE2_Csharp_PER 11368
#define TONE3_A_PER 7161
#define TONE4_Elow_PER 19118

void buzzer_on(const uint8_t tone) { // Tutorial 12
    spi_init();
    static const uint16_t periods[4] = {TONE1_Ehigh_PER, TONE2_Csharp_PER, TONE3_A_PER, TONE4_Elow_PER};
    if (tone == 0) {
        // spi_write(0b10111110);
        TCA0.SINGLE.PERBUF = periods[0];
        TCA0.SINGLE.CMP1BUF = periods[0];
        TCA0.SINGLE.CMP0BUF = (TCA0.SINGLE.PERBUF >> 1);
    }
    else if (tone == 1) {
        // spi_write(0b11101011);
        TCA0.SINGLE.PERBUF = periods[1];
        TCA0.SINGLE.CMP1BUF = periods[1];
        TCA0.SINGLE.CMP0BUF = (TCA0.SINGLE.PERBUF >> 1);
    }
    else if (tone == 2) {
        // spi_write(0b00111110);
        TCA0.SINGLE.PERBUF = periods[2];
        TCA0.SINGLE.CMP1BUF = periods[2];
        TCA0.SINGLE.CMP0BUF = (TCA0.SINGLE.PERBUF >> 1);
    }
    else if (tone == 3) {
        // spi_write(0b01101011);
        TCA0.SINGLE.PERBUF = periods[3];
        TCA0.SINGLE.CMP1BUF = periods[3];
        TCA0.SINGLE.CMP0BUF = (TCA0.SINGLE.PERBUF >> 1);
    }
}

void buzzer_off(void) {
    TCA0_SINGLE_CMP1BUF = 0;
    TCA0.SINGLE.CMP0BUF = 0;
}