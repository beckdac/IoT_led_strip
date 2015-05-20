#ifndef _TIMER0_H_
#define _TIMER0_H_

#define TIMER0_OVERFLOWS_PER_SECOND 77

void timer0_init();
void timer0_stop();
void timer0_start();

void rgb_init();
void rgb_set(uint8_t red, uint8_t green, uint8_t blue);

#endif /* _TIMER0_H_ */
