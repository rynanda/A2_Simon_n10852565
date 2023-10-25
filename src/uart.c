
// Include headers
#include <avr/io.h>
#include <avr/interrupt.h>

// Initialise UART
void uart_init(void) {
    PORTB.DIRSET = PIN2_bm;                             // Enable PB2 as output (USART0 TXD)
    USART0.BAUD = 1389;                                 // 9600 baud @ 3.3 MHz - 8N1
    USART0.CTRLA = USART_RXCIE_bm;                      // Enable RX interrupts
    USART0.CTRLB = USART_RXEN_bm | USART_TXEN_bm;       // Enable Tx/Rx
}

// Print single character to UART
void uart_putc(uint8_t c){
    while (!(USART0.STATUS & USART_DREIF_bm));          
    USART0.TXDATAL = c; 
}

// Print string (char array) to UART
void uart_puts(char* string) {
    const char *ptr = string;
    while (*ptr != '\0') {
        uart_putc(*ptr);
        ptr++;
    }
}