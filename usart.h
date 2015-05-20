#ifndef _UART_H_
#define _UART_H_

#include "circbuf.h"

#define BAUD_TO_UBRR(baud) F_CPU/16/baud-1

typedef struct usart {
	circbuf_t TX_buffer;
	circbuf_t RX_buffer;
} usart_t;
	
void usart_init(uint16_t ubrr);
void usart_send(uint8_t data);
uint8_t usart_receive(void);
int8_t usart_send_buffer(uint8_t size, uint8_t *buffer);
int8_t usart_receive_buffer(uint8_t size, uint8_t *buffer);
int usart_putchar(char c, FILE *stream);
int usart_getchar(FILE *stream);

#endif /* _UART_H_ */
