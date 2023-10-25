
// Include headers
#include <stdint.h>

// Debounced state of buttons to be read in Simon.c
extern volatile uint8_t pb_debounced_state;

// Function prototypes for pushbuttons initialisation and timer
void buttons_init(void);
void buttons_timer(void);
