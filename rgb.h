#ifndef _RGB_H_
#define _RGB_H_

#define RGB_DEFAULT_GOVERNOR 250

#define LED_RED_DIR     DDRD
#define LED_RED_PORT    PORTD
#define LED_RED_PIN     PD5
#define LED_RED_PORTP   PORTD5
#define LED_RED_INIT    LED_RED_DIR |= (1 << LED_RED_PIN)
#define LED_RED_LOW     LED_RED_PORT &= ~(1 << LED_RED_PORTP)
#define LED_RED_HIGH    LED_RED_PORT |= (1 << LED_RED_PORTP)

#define LED_BLUE_DIR   DDRD
#define LED_BLUE_PORT  PORTD
#define LED_BLUE_PIN   PD6
#define LED_BLUE_PORTP PORTD6
#define LED_BLUE_INIT  LED_BLUE_DIR |= (1 << LED_BLUE_PIN)
#define LED_BLUE_LOW   LED_BLUE_PORT &= ~(1 << LED_BLUE_PORTP)
#define LED_BLUE_HIGH  LED_BLUE_PORT |= (1 << LED_BLUE_PORTP)

#define LED_GREEN_DIR    DDRD
#define LED_GREEN_PORT   PORTD
#define LED_GREEN_PIN    PD7
#define LED_GREEN_PORTP  PORTD7
#define LED_GREEN_INIT   LED_GREEN_DIR |= (1 << LED_GREEN_PIN)
#define LED_GREEN_LOW    LED_GREEN_PORT &= ~(1 << LED_GREEN_PORTP)
#define LED_GREEN_HIGH   LED_GREEN_PORT |= (1 << LED_GREEN_PORTP)

void rgb_init(void);
void rgb_set(uint8_t red, uint8_t green, uint8_t blue);
void rgb_set_governor(uint8_t govna);
void rgb_rainbow(void);
void rgb_test(void);

#endif /* _RGB_H_ */
