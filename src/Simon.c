
// Include headers
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdint.h>
#include "uart.h"
#include "spi.h"
#include "timer.h"
#include "pwm.h"
#include "adc.h"
#include "buttons.h"
#include "tone.h"

// Initialise global variables
uint32_t student_num = 0x10852565;          // Save student number seed for resetting     
uint32_t STATE_LSFR = 0x10852565;           // Pseudo-random sequence seeded by student number
uint32_t new_seed = 0;                      // Variable to store new seeds
uint8_t STEP;                               // STEP variable to generate next sequence based on STATE_LSFR
uint8_t to_play;                            // Which tone to play - 1 = E(high), 2 = C#, 3 = A, 4 = E(low)
uint8_t result;                             // Store potentiometer reading
uint16_t sequence_length = 1;               // Same as user score
uint16_t played = 0;                        // How many tones has been played in current game, also used for indexing
uint8_t correct_tones[];                    // Array to store correct tone at each step
uint8_t user_tones[];                       // Array to store user tone at each step
uint8_t *c = correct_tones;                 // Pointer to correct_tones array
uint8_t *u = user_tones;                    // Pointer to user_tones array
static uint32_t playback_delay;             // Variable to store playback_delay, 0.25s (250ms) - 2s (2000ms)
static int user_success = 0;                // Flag to signify user success playing tone at each step, 0 - fail, 1 - success
static int digit_disp = 0;                  // Which side of the 7-seg to display, 0 - right, 1 - left
char rx;                                    // Variable to store character received through UART
char first[21];                             // Char array to store #1 name in high score table
char second[21];                            // Char array to store #2 name in high score table
char third[21];                             // Char array to store #3 name in high score table
char fourth[21];                            // Char array to store #4 name in high score table
char fifth[21];                             // Char array to store #5 name in high score table
char* high_score_names[];                   // Array of names in the high score table
uint16_t high_scores[5];                    // uint16_t array to store high scores
static int high_scores_num = 1;             // Flag to signify which entry of high score table to update
static int recv_high_scores = 0;            // Flag to signify if a high score name is being received
uint16_t ones_score;                        // User score to show in ones place
uint16_t tens_score;                        // User score to show in tens place

// Macros for 7-segment display
#define TONESEG_1 0b10111110                // First vertical for tone 1
#define TONESEG_2 0b11101011                // Second vertical for tone 2
#define TONESEG_3 0b00111110                // Third vertical for tone 3
#define TONESEG_4 0b01101011                // Fourth vertical for tone 4
#define SUCCESS_RIGHT 0b00000000            // Success pattern on left hand side
#define SUCCESS_LEFT 0b10000000             // Success pattern on right hand side
#define FAIL_RIGHT 0b01110111               // Fail pattern on right hand side
#define FAIL_LEFT 0b11110111                // Fail pattern on left hand side
#define ZERO_RIGHT 0b00001000               // Display zero on right hand side
#define ZERO_LEFT 0b10001000                // Display zero on left hand side
#define ONE_RIGHT 0b01101011                // Display one on right hand side
#define ONE_LEFT 0b11101011                 // Display one on left hand side
#define TWO_RIGHT 0b01000100                // Display twp on right hand side
#define TWO_LEFT 0b11000100                 // Display two on left hand side
#define THREE_RIGHT 0b01000001              // Display three on right hand side
#define THREE_LEFT 0b11000001               // Display three on left hand side
#define FOUR_RIGHT 0b00100011               // Display four on right hand side
#define FOUR_LEFT 0b10100011                // Display four on left hand side
#define FIVE_RIGHT 0b00010001               // Display five on right hand side
#define FIVE_LEFT 0b10010001                // Display five on left hand side
#define SIX_RIGHT 0b00010000                // Display six on right hand side
#define SIX_LEFT 0b10010000                 // Display six on left hand side
#define SEVEN_RIGHT 0b01001011              // Display seven on right hand side
#define SEVEN_LEFT 0b11001011               // Display seven on left hand side
#define EIGHT_RIGHT 0b00000000              // Display eight on right hand side
#define EIGHT_LEFT 0b10000000               // Display eight on left hand side
#define NINE_RIGHT 0b00000011               // Display nine on right hand side
#define NINE_LEFT 0b10000011                // Display nine on left hand side

// Typedef for tone states
typedef enum {
    WAIT = 0,
    TONE1_Ehigh = 1,
    TONE2_Csharp = 2,
    TONE3_A = 3,
    TONE4_Elow = 4
} TONE_STATES;

TONE_STATES tone_state = WAIT;              // Initialise tone state

// Typedef for playing states
typedef enum {
    NOT_PLAYING,
    PLAYING,
    SUCCESS_DISP,
    FAIL_DISP,
    USER_SCORE,
    HIGH_SCORES
} PLAYING_STATES;

PLAYING_STATES playing_state = NOT_PLAYING; // Initialise playing state

// Typedef for UART symbol states
typedef enum {
    AWAIT_SYM,
    S1_SYM,
    S2_SYM,
    S3_SYM,
    S4_SYM,
    INC_FREQ,
    DEC_FREQ,
    SEED,
    RESET,
    FIRST,
    SECOND,
    THIRD,
    FOURTH,
    FIFTH,
    END
} SYM_STATES;

SYM_STATES sym_state = AWAIT_SYM;           // Initialise symbol state

// Generate next sequence
void next(void) {
    int BIT = (STATE_LSFR & 1);             // BIT <- lsbit(STATE_LSFR)
    STATE_LSFR >>= 1;                       // STATE_LSFR <- STATE_LSFR >> 1
    if (BIT == 1) {
        STATE_LSFR ^= 0xE2023CAB;           // STATE_LSFR <- STATE_LSFR xor MASK (0xE2023CAB)
    }
    STEP = STATE_LSFR & 0b11;               // STEP <- STATE_LSFR and 0b11
}

// Find which tone to play
void tone_sequence(uint8_t *number) {
    int lsb = (*number & 1);                    // Get least significant bit
    int lsb2 = (*number & (1 << 1));            // Get second least significant bit
    if ((lsb == 0) && (lsb2 == 0)) {
        to_play = 1;                            // E(high) to be played if STEP == 00
    } else if ((lsb == 1) && (lsb2 == 0)) {
        to_play = 2;                            // C sharp to be played if STEP == 01
    } else if ((lsb == 0) && (lsb2 == 2)) {
        to_play = 3;                            // A to be played if STEP == 10
    } else if ((lsb == 1) && (lsb2 == 2)) {
        to_play = 4;                            // E(low) to be played if STEP = 11
    }
}

// Function to reset the seed to either the student number or the new seed (if available)
void reset_seed(void) {
    if (new_seed == 0) {
        STATE_LSFR = student_num;
    }
    else STATE_LSFR = new_seed;
}

/* Function called after a tone has been played in the NOT_PLAYING state.
   Either continues playing if not enough tones has been played, 
   or stops and transitions to PLAYING state. */
void played_tone(void) {
    tone_off();
    if ((count >= (playback_delay >> 1))) {             // Stops playing for half of the playback delay
        next();
        tone_sequence(&STEP);                           // Obtain next tone to play
        tone_state = WAIT;                              // Transition back to waiting state
        count = 0;                                      // Reset counter
        played++;                                       // Increase number of tones played by one
        if (played == sequence_length) {                // If enough tones has been played, transition to PLAYING state
            playing_state = PLAYING;
            tone_state = WAIT;
            played = 0;
            reset_seed();
            tone_off();
        }
    }
}

// Function called after a user enters a tone
void user_tone(const uint8_t tone_played) {
    sym_state = AWAIT_SYM;                              // Reset receiving symbol state in the USART ISR
    u[played] = tone_played;                            // Store tone played to user tones array
    if (u[played] == c[played]) {
        user_success = 1;                               // If the tone is correct, the user succeeds
    }
    else {
        user_success = 0;                               // User fails if incorrect
        count = 0;
        tone_off();
        playing_state = FAIL_DISP;                      // Transition to display failure pattern state
    }
    played++;                                           
    if (played == sequence_length) {                    // If enough tones has been played,
        if(user_success == 1) {
            count = 0;
            tone_off();
            playing_state = SUCCESS_DISP;               // Transition to success pattern state if user succeeds
        }
        else {
            count = 0;
            tone_off();
            playing_state = FAIL_DISP;                  // Transition to fail pattern state if user fails
        }
    }
    else {
        tone_off();
        tone_state = WAIT;                              // Transition back to wait for next user tone if not enough tones played
    }
}

int main(void) {
    // Initialise UART, SPI, PWM, ADC, pushbuttons, and timers
    cli();
    uart_init();
    spi_init();
    pwm_init();
    adc_init();
    buttons_init();
    timer_init();
    buttons_timer();
    sei();

    // Get next tone to play
    next();
    tone_sequence(&STEP);

    while(1) {
        // Playback delay calculation
        result = ADC0.RESULT;                                       // Get potentiometer reading
        playback_delay = ((uint32_t)1750 * (uint32_t)result);       
        playback_delay = (playback_delay >> (uint32_t)8);           // Scale to between 0-1750 ms (with the line above)
        playback_delay += 250;                                      // Scale to between 250-2000 ms

        // Debouncing
        uint8_t pb_state_prev;
        static uint8_t pb_state;
        uint8_t pb_changed;
        uint8_t pb_falling_edge;
        uint8_t pb_rising_edge;

        pb_state_prev = pb_state;                                   // Store previous state of pushbuttons
        pb_state = pb_debounced_state;                              // Get debounced pushbuttons state
        pb_changed = pb_state ^ pb_state_prev;                      // Detect change in pushbuttons state
        pb_falling_edge = pb_changed & ~pb_state;                   // Detect falling edge (press)
        pb_rising_edge = pb_changed & pb_state;                     // Detect rising edge (release)

        switch(playing_state) {
            case NOT_PLAYING:
                switch (tone_state) {
                    case WAIT:
                        /* For each tone to be played, turn appropriate buzzer  
                           and display on for half of playback delay, 
                           then transition to appropriate tone state and reset counter. */
                        if (to_play == 1) {
                            tone_on(0);
                            spi_write(TONESEG_1);
                            if ((count >= (playback_delay >> 1))) {
                                tone_state = TONE1_Ehigh;
                                count = 0;
                            }
                        }
                        else if (to_play == 2) {
                            tone_on(1);
                            spi_write(TONESEG_2);
                            if ((count >= (playback_delay >> 1))) {
                                tone_state = TONE2_Csharp;
                                count = 0;
                            }
                        }
                        else if (to_play == 3) {
                            tone_on(2);
                            spi_write(TONESEG_3);
                            if ((count >= (playback_delay >> 1))) {
                                tone_state = TONE3_A;
                                count = 0;
                            }
                        }
                        else if (to_play == 4) {
                            tone_on(3);
                            spi_write(TONESEG_4);
                            if ((count >= (playback_delay >> 1))) {
                                tone_state = TONE4_Elow;
                                count = 0;
                            }
                        }
                        break;
                        
                    /* Stops tone playing for half of playback delay.
                       Either continues playing if not enough tones has been played, 
                       or stops and transitions to PLAYING state. */
                    case TONE1_Ehigh:
                        played_tone();
                        break;
                    case TONE2_Csharp:
                        played_tone();
                        break;
                    case TONE3_A:
                        played_tone();
                        break;
                    case TONE4_Elow:
                        played_tone();
                        break;

                    // Default case to reset variables and transition back to NOT_PLAYING state
                    default:
                        playing_state = NOT_PLAYING;
                        tone_state = WAIT;
                        reset_seed();
                        tone_off();
                        reset_octave();
                        sequence_length = 1;
                        count = 0;
                        played = 0;
                }
                break;
            case PLAYING:
                /* Get the correct tones to play then store them in correct tones array, 
                   then prepare for user input */
                reset_seed();
                for (int i = 0; i < sequence_length; i++) {
                    next();
                    tone_sequence(&STEP);
                    c[i] = to_play;
                }
                reset_seed();
                next();
                tone_sequence(&STEP);
                count = 0;

                // Reset to wait for symbol in UART ISR after increasing/decreasing octave
                if ((sym_state == INC_FREQ) || (sym_state == DEC_FREQ)) {
                    sym_state = AWAIT_SYM;
                }

                switch(tone_state) {
                    case WAIT:
                        /* Detect user tones played, turn appropriate buzzer and display on
                           then transition to appropriate tone state */
                        if (pb_falling_edge & PIN4_bm) {
                            tone_state = TONE1_Ehigh;
                            tone_on(0);
                            spi_write(TONESEG_1);
                        }
                        else if (pb_falling_edge & PIN5_bm) {
                            tone_state = TONE2_Csharp;
                            tone_on(1);
                            spi_write(TONESEG_2);
                        }
                        else if (pb_falling_edge & PIN6_bm) {
                            tone_state = TONE3_A;
                            tone_on(2);
                            spi_write(TONESEG_3);
                        }
                        else if (pb_falling_edge & PIN7_bm) {
                            tone_state = TONE4_Elow;
                            tone_on(3);
                            spi_write(TONESEG_4);
                        }
                        break;

                    /* For each tone state, determine if user-inputted sequence is correct by comparing
                       to correct sequence, then transition either success or failure state
                       or transition back to wait for next user input if not enough tones has been played */
                    case TONE1_Ehigh:
                        if ((pb_rising_edge & PIN4_bm) || (sym_state == S1_SYM)) {
                            user_tone(1);
                        }
                        break;
                    case TONE2_Csharp:
                        if ((pb_rising_edge & PIN5_bm) || (sym_state == S2_SYM)) {
                            user_tone(2);
                        }
                        break;
                    case TONE3_A:
                        if ((pb_rising_edge & PIN6_bm) || (sym_state == S3_SYM)) {
                            user_tone(3);
                        }
                        break;
                    case TONE4_Elow:
                        if ((pb_rising_edge & PIN7_bm) || (sym_state == S4_SYM)) {
                            user_tone(4);
                        }
                        break;
                    }
                break;
            case SUCCESS_DISP:
                // Turn on display without turning on buzzer
                tone_success_fail();    

                // Alternate success pattern left and right for the playback delay
                if (digit_disp == 0) {
                    spi_write(SUCCESS_RIGHT);
                    digit_disp = 1;
                }
                else if (digit_disp == 1) {
                    spi_write(SUCCESS_LEFT);
                    digit_disp = 0;
                }

                // Transition back to NOT_PLAYING state to get next tone and continue playing game
                if ((count >= playback_delay)) {
                    playing_state = NOT_PLAYING;
                    tone_off();
                    tone_state = WAIT;
                    sequence_length++;                          // Increase sequence length/user score
                    uart_puts("SUCCESS\n");                     // Print SUCCESS string to UART
                    uart_putc(sequence_length + '0');           // Print score to UART
                    uart_putc('\n');
                    played = 0;
                    count = 0;
                }
                break;
            case FAIL_DISP:
                // Turn on display without turning on buzzer
                tone_success_fail();
                
                // Alternate success pattern left and right for the playback delay
                if (digit_disp == 0) {
                    spi_write(FAIL_RIGHT);
                    digit_disp = 1;
                }
                else if (digit_disp == 1) {
                    spi_write(FAIL_LEFT);
                    digit_disp = 0;
                }

                // Transition to show user score state
                if ((count >= playback_delay)) {
                    playing_state = USER_SCORE;
                    uart_puts("GAME OVER\n");                   // Print GAME OVER string to UART
                    uart_putc(sequence_length + '0');           // Print score to UART
                    uart_putc('\n');
                    tone_off();
                    count = 0;
                }
                break;
            case USER_SCORE:
                // Get user score to display in ones place and tens place
                if (sequence_length < 10) {
                    ones_score = sequence_length;
                    tens_score = 0;
                }
                else {
                    ones_score = sequence_length;
                    while (ones_score >= 10) {
                        ones_score = ones_score - 10;
                        tens_score = tens_score + 1;
                    }
                }

                // Alternate tens and ones of user score to display on left and right of display for the playback delay
                if (count < playback_delay) {    
                    tone_success_fail();
                    if (digit_disp == 0) {
                        if (ones_score == 0) {
                            spi_write(ZERO_RIGHT);
                        }
                        else if (ones_score == 1) {
                            spi_write(ONE_RIGHT);
                        }
                        else if (ones_score == 2) {
                            spi_write(TWO_RIGHT);
                        }
                        else if (ones_score == 3) {
                            spi_write(THREE_RIGHT);
                        }
                        else if (ones_score == 4) {
                            spi_write(FOUR_RIGHT);
                        }
                        else if (ones_score == 5) {
                            spi_write(FIVE_RIGHT);
                        }
                        else if (ones_score == 6) {
                            spi_write(SIX_RIGHT);
                        }
                        else if (ones_score == 7) {
                            spi_write(SEVEN_RIGHT);
                        }
                        else if (ones_score == 8) {
                            spi_write(EIGHT_RIGHT);
                        }
                        else if (ones_score == 9) {
                            spi_write(NINE_RIGHT);
                        }
                        digit_disp = 1;
                    }
                    else if (digit_disp == 1) {
                        if (tens_score == 0) {
                            spi_write(ZERO_LEFT);
                        }
                        else if (tens_score == 1) {
                            spi_write(ONE_LEFT);
                        }
                        else if (tens_score == 2) {
                            spi_write(TWO_LEFT);
                        }
                        else if (tens_score == 3) {
                            spi_write(THREE_LEFT);
                        }
                        else if (tens_score == 4) {
                            spi_write(FOUR_LEFT);
                        }
                        else if (tens_score == 5) {
                            spi_write(FIVE_LEFT);
                        }
                        else if (tens_score == 6) {
                            spi_write(SIX_LEFT);
                        }
                        else if (tens_score == 7) {
                            spi_write(SEVEN_LEFT);
                        }
                        else if (tens_score == 8) {
                            spi_write(EIGHT_LEFT);
                        }
                        else if (tens_score == 9) {
                            spi_write(NINE_LEFT);
                        }
                        digit_disp = 0;
                    }
                }

                /* Prompt user for name and transition to HIGH_SCORES state after turning off 
                   buzzer and display for the playback delay */
                if ((count >= (playback_delay * 2))) {
                    playing_state = HIGH_SCORES;
                    recv_high_scores = 1;                       // Set receiving high score name flag to get name
                    uart_puts("Enter name: ");
                    tone_off();
                    count = 0;
                }
                else if (count >= playback_delay) {
                    tone_off();
                }
                break;
            case HIGH_SCORES:
                // Once high score name has been received, store in appropriate index of high score names array
                if (recv_high_scores == 0) {
                    if (high_scores_num == 1) {
                        high_score_names[high_scores_num - 1] = first;
                    }
                    else if (high_scores_num == 2) {
                        high_score_names[high_scores_num - 1] = second;
                    }
                    else if (high_scores_num == 3) {
                        high_score_names[high_scores_num - 1] = third;
                    }
                    else if (high_scores_num == 4) {
                        high_score_names[high_scores_num - 1] = fourth;
                    }
                    else if (high_scores_num == 5) {
                        high_score_names[high_scores_num - 1] = fifth;
                    }
                    high_scores[high_scores_num - 1] = sequence_length;             // Store user score in high scores array
                    uart_putc('\n');

                    // Print high score table to UART
                    for (int i = 0; i < high_scores_num; i++) {
                        uart_puts(high_score_names[i]);
                        uart_puts(" ");
                        uart_putc(high_scores[i] + '0');
                        uart_putc('\n');
                    }

                    // Start a new game
                    high_scores_num++;
                    playing_state = NOT_PLAYING;
                    reset_seed();
                    tone_off();
                    tone_state = WAIT;
                    sequence_length = 1;
                    played = 0;
                    count = 0;
                }
                break;

            // Default case to reset game
            default:
                playing_state = NOT_PLAYING;
                tone_state = WAIT;
                reset_seed();
                tone_off();
                reset_octave();
                sequence_length = 1;
                count = 0;
                played = 0;
        }
    }
}

// Macro to signify user is not inputting seed or high score name
#define NOT_RECEIVING_INPUT 0xFF;

// USART ISR
ISR(USART0_RXC_vect) {
    rx = USART0.RXDATAL;                                        // Get user-inputted symbol
    static char rxbuf[9];                                       // Received symbol buffer for new seed
    static uint8_t rxpos = NOT_RECEIVING_INPUT;                 // Index for new seed/high score name

    // If not currently receiving high score name, 
    if (recv_high_scores == 0) {
        switch (sym_state) {
            case AWAIT_SYM:
                // User plays tone 1
                if ((rx == 'q') || (rx == '1')) {
                    sym_state = S1_SYM;
                    tone_state = TONE1_Ehigh;
                    tone_on(0);
                    spi_write(TONESEG_1);
                }

                // User plays tone 2
                else if ((rx == 'w') || (rx == '2')) {
                    sym_state = S2_SYM;
                    tone_state = TONE2_Csharp;
                    tone_on(1);
                    spi_write(TONESEG_2);
                }

                // User plays tone 3
                else if ((rx == 'e') || (rx == '3')) {
                    sym_state = S3_SYM;
                    tone_state = TONE3_A;
                    tone_on(2);
                    spi_write(TONESEG_3);
                }

                // User plays tone 4
                else if ((rx == 'r') || (rx == '4')) {
                    sym_state = S4_SYM;
                    tone_state = TONE4_Elow;
                    tone_on(3);
                    spi_write(TONESEG_4);
                }

                // User increases octave
                else if ((rx == ',') || (rx == 'k')) {
                    sym_state = INC_FREQ;
                    inc_octave();
                }

                // User decreases octave
                else if ((rx == '.') || (rx == 'l')) {
                    sym_state = DEC_FREQ;
                    dec_octave();
                }

                // User wants new seed
                else if ((rx == '9') || (rx == 'o')) {
                    sym_state = SEED;
                    rxpos = 0;                                  // Initialise seed index
                }

                // Reset game
                else if ((rx == '0') || (rx == 'p')) {
                    sym_state = RESET;
                    reset_octave();
                    sequence_length = 1;
                    playing_state = NOT_PLAYING;
                    tone_state = WAIT;
                    played = 0;
                    count = 0;
                }
                break;

            /* For each state, reset rx symbol to prepare for next user input 
               Symbol state is reset in main function */
            case S1_SYM:
                rx = '\0';
                break;
            case S2_SYM:
                rx = '\0';
                break;
            case S3_SYM:
                rx = '\0';
                break;
            case S4_SYM:
                rx = '\0';
                break;
            case INC_FREQ:
                rx = '\0';
                break;
            case DEC_FREQ:
                rx = '\0';
                break;

            // Get new seed
            case SEED:
                rxbuf[rxpos++] = rx;                                // Add each new symbol to buffer
                if (rxpos == 8) {                                   // Once 8 characters of seed has been obtained,
                    rxpos = NOT_RECEIVING_INPUT;                    // Reset rxpos
                    rxbuf[8] = '\0';                                // Set last character in array to null character
                    if (sscanf(rxbuf, "%x", &new_seed) == 1) {
                        STATE_LSFR = new_seed;                      // Convert seed from string to hex
                    }
                    sym_state = AWAIT_SYM;                          // Transition back to wait for symbol
                    rx = '\0';                                      // Reset rx symbol ready for next user input
                }
                break;

            /* Reset rx symbol to prepare for next user input 
               Symbol state is reset in main function */
            case RESET:
                rx = '\0';
                break;
        }
    }

    // Receiving high score name
    else if (recv_high_scores != 0) {
        switch (sym_state) {
            case AWAIT_SYM:
                // Get first letter of name and store in array based on which position in high score table
                if (high_scores_num == 1) {
                    sym_state = FIRST;
                    first[0] = rx;
                }
                else if (high_scores_num == 2) {
                    sym_state = SECOND;
                    second[0] = rx;
                }
                else if (high_scores_num == 3) {
                    sym_state = THIRD;
                    third[0] = rx;
                }
                else if (high_scores_num == 4) {
                    sym_state = FOURTH;
                    fourth[0] = rx;
                }
                else if (high_scores_num == 5) {
                    sym_state = FIFTH;
                    fifth[0] = rx;
                }
                rxpos = 1;
                break;

            /* For the following states FIRST, SECOND, THIRD, FOURTH, FIFTH,
               Continue getting letters of name until either newline character
               received (symbolising ENTER key) or 20 letters has been obtained. */
            case FIRST:
                if (rx == '\n') {
                    first[rxpos] = '\0';                    // Set last letter as null character
                    rxpos = NOT_RECEIVING_INPUT;            // Reset rxpos
                    sym_state = AWAIT_SYM;                  // Transition back to wait for next user input symbol
                    rx = '\0';                              // Reset rx
                    recv_high_scores = 0;                   // Set flag to 0 to signify name has been obtained
                }
                else if (rxpos == 20) {
                    rxpos = NOT_RECEIVING_INPUT;
                    first[20] = '\0';
                    sym_state = AWAIT_SYM;
                    rx = '\0';
                    recv_high_scores = 0;
                }
                else first[rxpos++] = rx;
                break;
            case SECOND:
                if (rx == '\n') {
                    second[rxpos] = '\0';
                    rxpos = NOT_RECEIVING_INPUT;
                    sym_state = AWAIT_SYM;
                    rx = '\0';
                    recv_high_scores = 0;
                }
                else if (rxpos == 20) {
                    rxpos = NOT_RECEIVING_INPUT;
                    second[20] = '\0';
                    sym_state = AWAIT_SYM;
                    rx = '\0';
                    recv_high_scores = 0;
                }
                else second[rxpos++] = rx;
                break;
            case THIRD:
                if (rx == '\n') {
                    third[rxpos] = '\0';
                    rxpos = NOT_RECEIVING_INPUT;
                    sym_state = AWAIT_SYM;
                    rx = '\0';
                    recv_high_scores = 0;
                }
                else if (rxpos == 20) {
                    rxpos = NOT_RECEIVING_INPUT;
                    third[20] = '\0';
                    sym_state = AWAIT_SYM;
                    rx = '\0';
                    recv_high_scores = 0;
                }
                else third[rxpos++] = rx;
                break;
            case FOURTH:
                if (rx == '\n') {
                    fourth[rxpos] = '\0';
                    rxpos = NOT_RECEIVING_INPUT;
                    sym_state = AWAIT_SYM;
                    rx = '\0';
                    recv_high_scores = 0;
                }
                else if (rxpos == 20) {
                    rxpos = NOT_RECEIVING_INPUT;
                    fourth[20] = '\0';
                    sym_state = AWAIT_SYM;
                    rx = '\0';
                    recv_high_scores = 0;
                }
                else fourth[rxpos++] = rx;
                break;
            case FIFTH:
                if (rx == '\n') {
                    fifth[rxpos] = '\0';
                    rxpos = NOT_RECEIVING_INPUT;
                    sym_state = AWAIT_SYM;
                    rx = '\0';
                    recv_high_scores = 0;
                }
                else if (rxpos == 20) {
                    rxpos = NOT_RECEIVING_INPUT;
                    fifth[20] = '\0';
                    sym_state = AWAIT_SYM;
                    rx = '\0';
                    recv_high_scores = 0;
                }
                else fifth[rxpos++] = rx;
                break;
        }
    }
}