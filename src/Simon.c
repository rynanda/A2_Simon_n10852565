
// Include headers
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdint.h>
#include "uart.h"
#include "spi.h"
// #include "timer.h"
#include "buttons.h"
#include "buzzer.h"
// #include "qutyserial.h"
// #include "initialisation.h"

// Initialise global variables
uint32_t STATE_LSFR = 0x10852565;           // Pseudo-random sequence seeded by student #
uint32_t STEP;
uint8_t to_play;
uint8_t result;
// uint16_t user_score = sequence_length;
uint8_t correct_tones[256];
uint8_t user_tones[256];
uint8_t *c = correct_tones;
uint8_t *u = user_tones;
static uint32_t playback_delay;
static uint16_t count = 0;
static int digit_disp = 0;
static char rx;

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
    USER_SCORE
} PLAYING_STATES;

PLAYING_STATES playing_state = NOT_PLAYING;

typedef enum {
    AWAIT_SYM = '\n',
    S1_SYM = 'q',
    S2_SYM = 'w',
    S3_SYM = 'e',
    S4_SYM = 'r'
} SYM_STATES;

SYM_STATES sym_state = AWAIT_SYM;

// Generate next sequence (tutorial 6)
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

void pwm_init(void) { // Tutorial 8
    PORTB_DIRSET = PIN1_bm | PIN0_bm;                   // Enable PB1 (DISP EN) and PB0 (BUZZER) as outputs
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc;      // CLK_PER select, /1 prescaler (3.33 MHz)
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc | TCA_SINGLE_CMP1EN_bm | TCA_SINGLE_CMP0EN_bm; // Single-slope PWM mode, WO1 and WO0 enable
    TCA0.SINGLE.PER = 7161;                             // 465 Hz (A) is ~7161.29 clocks @ 3.33 MHz
    TCA0.SINGLE.CMP1 = 0;                               // off initially, Maximum brightness -> 100% duty cycle
    TCA0.SINGLE.CMP0 = 0;                               // BUZZER off initially -> 0% duty cycle || Max loudness = 50% duty cycle
    TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;          // Enable TCA0

    /* 
        Modify with PERBUF, CMP1BUF, and CMP0BUF later based on next sequence/tone to play.
        E(high) = 465 ∗ 2^(−5/12) = 348.3563954 Hz ~ 348 Hz -> PERBUF = 9559, CMP1BUF = 9559, CMP0BUF = 9559/2
        C# = 465∗ 2^(−8/12) = 292.9316441 Hz ~ 293 Hz -> PERBUF = 11368, CMP1BUF = 11368, CMP0BUF = 11368/2
        A = 465 Hz -> PERBUF = 7161, CMP1BUF = 7161, CMP0BUF = 7161/2
        E(low) = 465∗ 2^(−17/12) = 174.1781977 Hz ~ 174 Hz -> PERBUF = 19118, CMP1BUF = 19118, CMP0BUF = 19118/2

        TONE1 | 465 * 2^(-5/12) = 348.3564 = 348    | Segments EF (LHS) = 0b10111110
        TONE2 | 465 * 2^(-8/12) = 292.9316 = 293    | Segments BC (LHS) = 0b11101011
        TONE3 | 465                                 | Segments EF (RHS) = 0b00111110
        TONE4 | 465 * 2^(-17/12) = 174.1782 = 174   | Segments BC (RHS) = 0b01101011
    */
}

void adc_init(void) { // Tutorial 8 // For playback delay -> scaling to between 0.25 and 2 seconds, increase delay = clockwise, vice versa
    ADC0.CTRLA = ADC_ENABLE_bm;                                         // Enable ADC
    ADC0.CTRLB = ADC_PRESC_DIV2_gc;                                     // /2 clock prescaler
    ADC0.CTRLC = (4 << ADC_TIMEBASE_gp) | ADC_REFSEL_VDD_gc;
    ADC0.CTRLE = 64;                                                    // Sample duration 64
    ADC0.CTRLF = ADC_FREERUN_bm;                                        // Free running mode
    ADC0.MUXPOS = ADC_MUXPOS_AIN2_gc;                                   // Select AIN2 (potentiometer R1)
    ADC0.COMMAND = ADC_MODE_SINGLE_8BIT_gc | ADC_START_IMMEDIATE_gc;    // 8-bit single conversion mode, start sampling immediately
}

void timer_init(void) { //  Tutorial 9
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;
    TCB0.CCMP = 3333;                   // Set interval for 1ms (3333 clocks @ 3.3 MHz)
    TCB0.INTCTRL = TCB_CAPT_bm;         // CAPT interrupt enable
    TCB0.CTRLA = TCB_ENABLE_bm;         // Enable
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

    static uint16_t played = 0;
    static uint16_t sequence_length = 1;                    // Same as user_score
    static int user_success = 0;

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

        // uart_putc((u[played] + '0'));

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
                                STATE_LSFR = 0x10852565;
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
                                STATE_LSFR = 0x10852565;
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
                                STATE_LSFR = 0x10852565;
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
                                STATE_LSFR = 0x10852565;
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
                STATE_LSFR = 0x10852565;
                for (int i = 0; i < sequence_length; i++) {
                    next();
                    tone_sequence(&STEP);
                    c[i] = to_play;
                }
                STATE_LSFR = 0x10852565;
                next();
                tone_sequence(&STEP);
                count = 0;

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
                    rx = '\0';
                    buzzer_off();
                    tone_state = WAIT;
                    sequence_length++;
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
                    playing_state = NOT_PLAYING;
                    STATE_LSFR = 0x10852565;
                    buzzer_off();
                    tone_state = WAIT;
                    sequence_length = 1;
                    played = 0;
                    count = 0;
                }
                break;
        }
    }
}

ISR(TCB0_INT_vect) { // EXT5 (?)
    
    count++;

    TCB0.INTFLAGS = TCB_CAPT_bm;        // Acknowledge interrupt
}

ISR(USART0_RXC_vect) {
    rx = USART0.RXDATAL;
    USART0.TXDATAL = rx;

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
        
    }
}