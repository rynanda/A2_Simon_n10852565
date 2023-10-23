
#ifndef BUZZER_H
#define BUZZER_H 1

#include <avr/io.h>
#include <avr/interrupt.h>

void buzzer_on(const uint8_t tone);
void buzzer_off(void);
void buzzer_success_fail(void);
void inc_octave(void);
void dec_octave(void);

#endif