
#include <avr/io.h>
#include <avr/interrupt.h>

void spi_init(void) {
    cli();
    PORTMUX.SPIROUTEA = PORTMUX_SPI0_ALT1_gc;   // SPI pins on PC0-3
    PORTC.DIR |= (PIN0_bm | PIN2_bm);           // Set SCK and MOSI as outputs

    SPI0.CTRLA = SPI_MASTER_bm;                 // Master, /4 prescaler, MSB first
    SPI0_CTRLB = SPI_SSD_bm;                    // Mode 0, client select disable, unbuffered
    SPI0_CTRLA |= SPI_ENABLE_bm;                // Enable

    PORTA_OUTCLR = PIN1_bm;                     // Set DISP LATCH low initially
    PORTA_DIR |= PIN1_bm;                       // Set DISP LATCH as output
    SPI0.INTCTRL = SPI_IE_bm;                   // Enable SPI interrupt
    sei();
}

void spi_write(uint8_t b) {
    SPI0.DATA = b;                              // DATA register used for both Tx and Rx
}

ISR(SPI0_INT_vect) {
    PORTA_OUTSET = PIN1_bm;                     // Drive PA1 (DISP LATCH) high
    SPI0_INTFLAGS ^= PIN7_bm;                   // Toggle off IF (bit 7) in SPI0.INTFLAGS to handle interrupt
}