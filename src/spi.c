
#include <avr/io.h>
#include <avr/interrupt.h>

void spi_init(void) {
    cli();
    PORTMUX.SPIROUTEA = PORTMUX_SPI0_ALT1_gc;   // SPI pins on PC0-3
    PORTC.DIRSET = PIN0_bm | PIN2_bm;           // Set SCK and MOSI as outputs

    SPI0.CTRLA = SPI_MASTER_bm;                 // Master, /4 prescaler, MSB first
    SPI0.CTRLB = SPI_SSD_bm;                    // Mode 0, client select disable, unbuffered
    SPI0.CTRLA |= SPI_ENABLE_bm;                // Enable

    PORTA.OUTCLR = PIN1_bm;                     // Set DISP LATCH low initially
    PORTA.DIRSET = PIN1_bm;                     // Set DISP LATCH as output
    SPI0.INTCTRL = SPI_IE_bm;                   // Enable SPI interrupt
    sei();
}

void spi_write(uint8_t b) {
    SPI0.DATA = b;                              // DATA register used for both Tx and Rx
}

ISR(SPI0_INT_vect) {
    PORTA.OUTSET = PIN1_bm;                     // Drive PA1 (DISP LATCH) high
    SPI0.INTFLAGS ^= SPI_IF_bm;                   // Toggle off IF (bit 7) in SPI0.INTFLAGS to handle interrupt
}