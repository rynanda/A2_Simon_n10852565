
#include <avr/io.h>
#include <avr/interrupt.h>

extern volatile uint8_t pb_debounced_state;

void buttons_init(void);