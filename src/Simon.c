
// Include headers
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdint.h>
#include "uart.h"
#include "spi.h"
#include "timer.h"
#include "pwm.h"
#include "adc.h"
#include "buttons.h"
#include "buzzer.h"

// Initialise global variables
uint32_t student_num = 0x10852565;
static uint32_t STATE_LSFR = 0x10852565;           // Pseudo-random sequence seeded by student #
static uint32_t new_seed = 0;
uint32_t STEP;
uint8_t to_play;
uint8_t result;
static uint16_t sequence_length = 1;                    // Same as user_score
static uint16_t played = 0;
uint8_t correct_tones[256];
uint8_t user_tones[256];
uint8_t *c = correct_tones;
uint8_t *u = user_tones;
int ones_score;
int tens_score;
static int user_success = 0;
static uint32_t playback_delay;
static int digit_disp = 0;
static char rx;
static char first[21];
static char second[21];
static char third[21];
static char fourth[21];
static char fifth[21];
char* high_score_names[];
static int high_scores[5];
static int high_scores_num = 1;
static int recv_high_scores = 0;

typedef enum { // Tutorial 10, Tutorial 12
    WAIT = 0,
    TONE1_Ehigh = 1,
    TONE2_Csharp = 2,
    TONE3_A = 3,
    TONE4_Elow = 4
} TONE_STATES;

TONE_STATES tone_state = WAIT;

typedef enum {
    NOT_PLAYING,
    PLAYING,
    SUCCESS_DISP,
    FAIL_DISP,
    USER_SCORE,
    HIGH_SCORES
} PLAYING_STATES;

PLAYING_STATES playing_state = NOT_PLAYING;

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

SYM_STATES sym_state = AWAIT_SYM;

// Generate next sequence
void next(void) {
    int BIT = (STATE_LSFR & 1);             // BIT <- lsbit(STATE_LSFR)
    STATE_LSFR >>= 1;                       // STATE_LSFR <- STATE_LSFR >> 1
    if (BIT == 1) {
        STATE_LSFR ^= 0xE2023CAB;           // STATE_LSFR <- STATE_LSFR xor MASK (0xE2023CAB)
    }
    STEP = STATE_LSFR & 0b11;               // STEP <- STATE_LSFR and 0b11
}

void tone_sequence(uint32_t *number) {
    int lsb = (*number & 1);
    int lsb2 = (*number & (1 << 1));
    if ((lsb == 0) && (lsb2 == 0)) {
        to_play = 1;    // E(high)
    } else if ((lsb == 1) && (lsb2 == 0)) {
        to_play = 2;    // C sharp
    } else if ((lsb == 0) && (lsb2 == 2)) {
        to_play = 3;    // A
    } else if ((lsb == 1) && (lsb2 == 2)) {
        to_play = 4;    // E(low)
    }
}

int main(void) {
    
    cli();
    uart_init();
    spi_init();
    pwm_init();
    adc_init();
    buttons_init();
    timer_init();
    buttons_timer();
    sei();

    next();
    tone_sequence(&STEP);

    while(1) {
        result = ADC0.RESULT;                                   // Get potentiometer reading (w/o reversing direction like TUT8)
        playback_delay = ((uint32_t)1750 * (uint32_t)result); 
        playback_delay = (playback_delay >> (uint32_t)8);       // Scale to between 0-1750 ms (with the line above)
        playback_delay += 250;                                  // Scale to between 250-2000 ms
        uint8_t pb_state_prev;
        static uint8_t pb_state;
        uint8_t pb_changed;
        uint8_t pb_falling_edge;
        uint8_t pb_rising_edge;

        pb_state_prev = pb_state;
        pb_state = pb_debounced_state;

        pb_changed = pb_state ^ pb_state_prev;

        pb_falling_edge = pb_changed & ~pb_state;
        pb_rising_edge = pb_changed & pb_state;

        switch(playing_state) {
            case NOT_PLAYING:
                switch (tone_state) {
                    case WAIT:
                        if (to_play == 1) {
                            buzzer_on(0);
                            spi_write(0b10111110);
                            if ((count >= (playback_delay >> 1))) {
                                tone_state = TONE1_Ehigh;
                                count = 0;
                            }
                        }
                        else if (to_play == 2) {
                            buzzer_on(1);
                            spi_write(0b11101011);
                            if ((count >= (playback_delay >> 1))) {
                                tone_state = TONE2_Csharp;
                                count = 0;
                            }
                        }
                        else if (to_play == 3) {
                            buzzer_on(2);
                            spi_write(0b00111110);
                            if ((count >= (playback_delay >> 1))) {
                                tone_state = TONE3_A;
                                count = 0;
                            }
                        }
                        else if (to_play == 4) {
                            buzzer_on(3);
                            spi_write(0b01101011);
                            if ((count >= (playback_delay >> 1))) {
                                tone_state = TONE4_Elow;
                                count = 0;
                            }
                        }
                        break;
                    case TONE1_Ehigh:
                        buzzer_off();
                        if ((count >= (playback_delay >> 1))) {
                            next();
                            tone_sequence(&STEP);
                            tone_state = WAIT;
                            count = 0;
                            played++;
                            if (played == sequence_length) {
                                playing_state = PLAYING;
                                tone_state = WAIT;
                                played = 0;
                                if (new_seed == 0) {
                                    STATE_LSFR = 0x10852565;
                                }
                                else STATE_LSFR = new_seed;
                                buzzer_off();
                            }
                        }
                        break;
                    case TONE2_Csharp:
                        buzzer_off();
                        if ((count >= (playback_delay >> 1))) {
                            next();
                            tone_sequence(&STEP);
                            tone_state = WAIT;
                            count = 0;
                            played++;
                            if (played == sequence_length) {
                                playing_state = PLAYING;
                                tone_state = WAIT;
                                played = 0;
                                if (new_seed == 0) {
                                    STATE_LSFR = 0x10852565;
                                }
                                else STATE_LSFR = new_seed;
                                buzzer_off();
                            }
                        }
                        break;
                    case TONE3_A:
                        buzzer_off();
                        if ((count >= (playback_delay >> 1))) {
                            next();
                            tone_sequence(&STEP);
                            tone_state = WAIT;
                            count = 0;
                            played++;
                            if (played == sequence_length) {
                                playing_state = PLAYING;
                                tone_state = WAIT;
                                played = 0;
                                if (new_seed == 0) {
                                    STATE_LSFR = 0x10852565;
                                }
                                else STATE_LSFR = new_seed;
                                buzzer_off();
                            }
                        }
                        break;
                    case TONE4_Elow:
                        buzzer_off();
                        if ((count >= (playback_delay >> 1))) {
                            next();
                            tone_sequence(&STEP);
                            tone_state = WAIT;
                            count = 0;
                            played++;
                            if (played == sequence_length) {
                                playing_state = PLAYING;
                                tone_state = WAIT;
                                played = 0;
                                if (new_seed == 0) {
                                    STATE_LSFR = 0x10852565;
                                }
                                else STATE_LSFR = new_seed;
                                buzzer_off();
                            }
                        }
                        break;
                    default:
                        buzzer_off();
                        count = 0;
                }
                break;
            case PLAYING:
                if (new_seed == 0) {
                    STATE_LSFR = 0x10852565;
                }
                else STATE_LSFR = new_seed;
                for (int i = 0; i < sequence_length; i++) {
                    next();
                    tone_sequence(&STEP);
                    c[i] = to_play;
                }
                if (new_seed == 0) {
                    STATE_LSFR = 0x10852565;
                }
                else STATE_LSFR = new_seed;
                next();
                tone_sequence(&STEP);
                count = 0;
                if ((sym_state == INC_FREQ) || (sym_state == DEC_FREQ)) {
                    sym_state = AWAIT_SYM;
                }

                switch(tone_state) {
                    case WAIT:
                        if (pb_falling_edge & PIN4_bm) {
                            tone_state = TONE1_Ehigh;
                            buzzer_on(0);
                            spi_write(0b10111110);
                        }
                        else if (pb_falling_edge & PIN5_bm) {
                            tone_state = TONE2_Csharp;
                            buzzer_on(1);
                            spi_write(0b11101011);
                        }
                        else if (pb_falling_edge & PIN6_bm) {
                            tone_state = TONE3_A;
                            buzzer_on(2);
                            spi_write(0b00111110);
                        }
                        else if (pb_falling_edge & PIN7_bm) {
                            tone_state = TONE4_Elow;
                            buzzer_on(3);
                            spi_write(0b01101011);
                        }
                        break;
                    case TONE1_Ehigh:
                        if ((pb_rising_edge & PIN4_bm) || (sym_state == S1_SYM)) {
                            sym_state = AWAIT_SYM;
                            u[played] = 1;
                            if (u[played] == c[played]) {
                                user_success = 1;
                            }
                            else {
                                user_success = 0;
                                count = 0;
                                buzzer_off();
                                playing_state = FAIL_DISP;
                            }
                            played++;
                            if (played == sequence_length) {
                                if(user_success == 1) {
                                    count = 0;
                                    buzzer_off();
                                    playing_state = SUCCESS_DISP;
                                }
                                else {
                                    count = 0;
                                    buzzer_off();
                                    playing_state = FAIL_DISP;
                                }
                            }
                            else {
                                buzzer_off();
                                tone_state = WAIT;
                            }
                        }
                        break;
                    case TONE2_Csharp:
                        if ((pb_rising_edge & PIN5_bm) || (sym_state == S2_SYM)) {
                            sym_state = AWAIT_SYM;
                            u[played] = 2;
                            if (u[played] == c[played]) {
                                user_success = 1;
                            }
                            else {
                                user_success = 0;
                                count = 0;
                                buzzer_off();
                                playing_state = FAIL_DISP;
                            }
                            played++;
                            if (played == sequence_length) {
                                if(user_success == 1) {
                                    count = 0;
                                    buzzer_off();
                                    playing_state = SUCCESS_DISP;
                                }
                                else {
                                    count = 0;
                                    buzzer_off();
                                    playing_state = FAIL_DISP;
                                }
                            }
                            else {
                                buzzer_off();
                                tone_state = WAIT;
                            }
                        }
                        break;
                    case TONE3_A:
                        if ((pb_rising_edge & PIN6_bm) || (sym_state == S3_SYM)) {
                            sym_state = AWAIT_SYM;
                            u[played] = 3;
                            if (u[played] == c[played]) {
                                user_success = 1;
                            }
                            else {
                                user_success = 0;
                                count = 0;
                                buzzer_off();
                                playing_state = FAIL_DISP;
                            }
                            played++;
                            if (played == sequence_length) {
                                if(user_success == 1) {
                                    count = 0;
                                    buzzer_off();
                                    playing_state = SUCCESS_DISP;
                                }
                                else {
                                    count = 0;
                                    buzzer_off();
                                    playing_state = FAIL_DISP;
                                }
                            }
                            else {
                                buzzer_off();
                                tone_state = WAIT;
                            }
                        }
                        break;
                    case TONE4_Elow:
                        if ((pb_rising_edge & PIN7_bm) || (sym_state == S4_SYM)) {
                            sym_state = AWAIT_SYM;
                            u[played] = 4;
                            if (u[played] == c[played]) {
                                user_success = 1;
                            }
                            else {
                                user_success = 0;
                                count = 0;
                                buzzer_off();
                                playing_state = FAIL_DISP;
                            }
                            played++;
                            if (played == sequence_length) {
                                if(user_success == 1) {
                                    count = 0;
                                    buzzer_off();
                                    playing_state = SUCCESS_DISP;
                                }
                                else {
                                    count = 0;
                                    buzzer_off();
                                    playing_state = FAIL_DISP;
                                }
                            }
                            else {
                                buzzer_off();
                                tone_state = WAIT;
                            }
                        }
                        break;
                    
                    }

                break;
            case SUCCESS_DISP:
                buzzer_success_fail();
                if (digit_disp == 0) {
                    spi_write(0b00000000);
                    digit_disp = 1;
                }
                else if (digit_disp == 1) {
                    spi_write(0b10000000);
                    digit_disp = 0;
                }
                if ((count >= playback_delay)) {
                    playing_state = NOT_PLAYING;
                    buzzer_off();
                    tone_state = WAIT;
                    sequence_length++;
                    uart_puts("SUCCESS\n");
                    uart_putc(sequence_length + '0');
                    uart_putc('\n');
                    played = 0;
                    count = 0;
                }
                break;
            case FAIL_DISP:
                buzzer_success_fail();
                if (digit_disp == 0) {
                    spi_write(0b01110111);
                    digit_disp = 1;
                }
                else if (digit_disp == 1) {
                    spi_write(0b11110111);
                    digit_disp = 0;
                }
                if ((count >= playback_delay)) {
                    playing_state = USER_SCORE;
                    uart_puts("GAME OVER\n");
                    uart_putc(sequence_length + '0');
                    uart_putc('\n');
                    buzzer_off();
                    count = 0;
                }
                break;
            case USER_SCORE:
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
                if (count < playback_delay) {    
                    buzzer_success_fail();
                    if (digit_disp == 0) {
                        if (ones_score == 0) {
                            spi_write(0b00001000);
                        }
                        else if (ones_score == 1) {
                            spi_write(0b01101011);
                        }
                        else if (ones_score == 2) {
                            spi_write(0b01000100);
                        }
                        else if (ones_score == 3) {
                            spi_write(0b01000001);
                        }
                        else if (ones_score == 4) {
                            spi_write(0b00100011);
                        }
                        else if (ones_score == 5) {
                            spi_write(0b00010001);
                        }
                        else if (ones_score == 6) {
                            spi_write(0b00010000);
                        }
                        else if (ones_score == 7) {
                            spi_write(0b01001011);
                        }
                        else if (ones_score == 8) {
                            spi_write(0b00000000);
                        }
                        else if (ones_score == 9) {
                            spi_write(0b00000011);
                        }
                        digit_disp = 1;
                    }
                    else if (digit_disp == 1) {
                        if (tens_score == 0) {
                            spi_write(0b10001000);
                        }
                        else if (tens_score == 1) {
                            spi_write(0b11101011);
                        }
                        else if (tens_score == 2) {
                            spi_write(0b11000100);
                        }
                        else if (tens_score == 3) {
                            spi_write(0b11000001);
                        }
                        else if (tens_score == 4) {
                            spi_write(0b10100011);
                        }
                        else if (tens_score == 5) {
                            spi_write(0b10010001);
                        }
                        else if (tens_score == 6) {
                            spi_write(0b10010000);
                        }
                        else if (tens_score == 7) {
                            spi_write(0b11001011);
                        }
                        else if (tens_score == 8) {
                            spi_write(0b10000000);
                        }
                        else if (tens_score == 9) {
                            spi_write(0b10000011);
                        }
                        digit_disp = 0;
                    }
                }
                if ((count >= (playback_delay * 2))) {
                    playing_state = HIGH_SCORES;
                    recv_high_scores = 1;
                    uart_puts("Enter name: ");
                    buzzer_off();
                    count = 0;
                }
                else if (count >= playback_delay) {
                    buzzer_off();
                }
                break;
            case HIGH_SCORES:
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
                    high_scores[high_scores_num - 1] = sequence_length;
                    uart_putc('\n');

                    for (int i = 0; i < high_scores_num; i++) {
                        uart_puts(high_score_names[i]);
                        uart_puts(" ");
                        uart_putc(high_scores[i] + '0');
                        uart_putc('\n');
                    }

                    high_scores_num++;
                    playing_state = NOT_PLAYING;
                    STATE_LSFR = 0x10852565;
                    buzzer_off();
                    tone_state = WAIT;
                    sequence_length = 1;
                    played = 0;
                    count = 0;
                }

        }
    }
}

#define NOT_RECEIVING_SEED 0xFF;
ISR(USART0_RXC_vect) {
    rx = USART0.RXDATAL;
    // USART0.TXDATAL = rx;
    static char rxbuf[9];
    static char prev;
    static uint8_t rxpos = NOT_RECEIVING_SEED;

    if (recv_high_scores == 0) {
        switch (sym_state) {
            case AWAIT_SYM:
                if ((rx == 'q') || (rx == '1')) {
                    sym_state = S1_SYM;
                    tone_state = TONE1_Ehigh;
                    buzzer_on(0);
                    spi_write(0b10111110);
                }
                else if ((rx == 'w') || (rx == '2')) {
                    sym_state = S2_SYM;
                    tone_state = TONE2_Csharp;
                    buzzer_on(1);
                    spi_write(0b11101011);
                }
                else if ((rx == 'e') || (rx == '3')) {
                    sym_state = S3_SYM;
                    tone_state = TONE3_A;
                    buzzer_on(2);
                    spi_write(0b00111110);
                }
                else if ((rx == 'r') || (rx == '4')) {
                    sym_state = S4_SYM;
                    tone_state = TONE4_Elow;
                    buzzer_on(3);
                    spi_write(0b01101011);
                }
                else if ((rx == ',') || (rx == 'k')) {
                    sym_state = INC_FREQ;
                    inc_octave();
                }
                else if ((rx == '.') || (rx == 'l')) {
                    sym_state = DEC_FREQ;
                    dec_octave();
                }
                else if ((rx == '9') || (rx == 'o')) {
                    sym_state = SEED;
                    rxpos = 0;
                }
                else if ((rx == '0') || (rx == 'p')) {
                    sym_state = RESET;
                    reset_octave();
                    sequence_length = 1;
                    playing_state = NOT_PLAYING;
                    tone_state = WAIT;
                    played = 0;
                    count = 0;
                }
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
            case SEED:
                rxbuf[rxpos++] = rx;
                if (rxpos == 8) {
                    rxpos = NOT_RECEIVING_SEED;
                    rxbuf[8] = '\0';
                    if (sscanf(rxbuf, "%x", &new_seed) == 1) {
                        STATE_LSFR = new_seed;
                    }
                    sym_state = AWAIT_SYM;
                    rx = '\0';
                }
                break;
            case RESET:
                rx = '\0';
                break;
        }
    }
    else if (recv_high_scores != 0) {
        switch (sym_state) {
            case AWAIT_SYM:
                prev = rx;
                if (high_scores_num == 1) {
                    sym_state = FIRST;
                    first[0] = prev;
                }
                else if (high_scores_num == 2) {
                    sym_state = SECOND;
                    second[0] = prev;
                }
                else if (high_scores_num == 3) {
                    sym_state = THIRD;
                    third[0] = prev;
                }
                else if (high_scores_num == 4) {
                    sym_state = FOURTH;
                    fourth[0] = prev;
                }
                else if (high_scores_num == 5) {
                    sym_state = FIFTH;
                    fifth[0] = prev;
                }
                rxpos = 1;
                break;
            case FIRST:
                if (rx == '\n') {
                    first[rxpos] = '\0';
                    rxpos = NOT_RECEIVING_SEED;
                    sym_state = AWAIT_SYM;
                    rx = '\0';
                    recv_high_scores = 0;
                }
                else if (rxpos == 20) {
                    rxpos = NOT_RECEIVING_SEED;
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
                    rxpos = NOT_RECEIVING_SEED;
                    sym_state = AWAIT_SYM;
                    rx = '\0';
                    recv_high_scores = 0;
                }
                else if (rxpos == 20) {
                    rxpos = NOT_RECEIVING_SEED;
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
                    rxpos = NOT_RECEIVING_SEED;
                    sym_state = AWAIT_SYM;
                    rx = '\0';
                    recv_high_scores = 0;
                }
                else if (rxpos == 20) {
                    rxpos = NOT_RECEIVING_SEED;
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
                    rxpos = NOT_RECEIVING_SEED;
                    sym_state = AWAIT_SYM;
                    rx = '\0';
                    recv_high_scores = 0;
                }
                else if (rxpos == 20) {
                    rxpos = NOT_RECEIVING_SEED;
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
                    rxpos = NOT_RECEIVING_SEED;
                    sym_state = AWAIT_SYM;
                    rx = '\0';
                    recv_high_scores = 0;
                }
                else if (rxpos == 20) {
                    rxpos = NOT_RECEIVING_SEED;
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