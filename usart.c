#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "global.h"
#include "usart.h"

FILE usart_output = FDEV_SETUP_STREAM(usart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE usart_input = FDEV_SETUP_STREAM(NULL, usart_getchar, _FDEV_SETUP_READ);

usart_t usart;

void usart_init(uint16_t ubrr) {
	circbuf_init(&usart.TX_buffer);
	circbuf_init(&usart.RX_buffer);

	/* clear out the registers as a fix up from the state optiboot left the usart in */
	UCSR0A = 0;
	UCSR0B = 0;	// not really necessary, see below
	UCSR0C = 0;	// not really necessary, see below

	/* Set the baud rate */
	UBRR0H = (ubrr >> 8);
   	UBRR0L = ubrr;

	/* enable transmit and receive circuitry */
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);

	/* set to 8 data bits, 1 stop bit */
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

	/* complete byte receive interrupt */
	UCSR0B |= (1 << RXCIE0);

	/* the UDRE interrupt is enabled on send */

	/* setup FDEV */
	stdout = &usart_output;
	stdin  = &usart_input;
}

/* no software buffer send and receive */
uint8_t usart_receive(void) {
	/* wait for incomming data */
	while (!(UCSR0A & (1 << RXC0)));
	return UDR0;
}

void usart_send(uint8_t data) {
	/* wait for empty transmit buffer */
	while (!(UCSR0A & (1 << UDRE0)));
	/* start transmittion */
	UDR0 = data;
}

/* for use with FDEV */
int usart_putchar(char c, FILE *stream) {
	if (c == '\n') {
		usart_putchar('\r', stream);
	}
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = c;

	return 1;
}

int usart_getchar(FILE *stream) {
	while (!(UCSR0A & (1 << RXC0)));
	return UDR0;
}

/* software buffered send and receive */
int8_t usart_send_buffer(uint8_t size, uint8_t *buffer) {
	if (!size)
		return FALSE;

	/* if there is room for the output buffer */
	if (usart.TX_buffer.length + size < BUFFER_SIZE) {
		uint8_t i;

		uint8_t sreg = SREG; 		/* save state */
		cli();				/* disable interrupts */

		/* copy the buffer to the transmit buffer */
		for (i = 0; i < size; ++i) {
			usart.TX_buffer.data[(usart.TX_buffer.idx + usart.TX_buffer.length) % BUFFER_SIZE] = buffer[i];
			usart.TX_buffer.length++;
		}

		SREG = sreg;			/* re-enable interrupts */

		UCSR0B |= (1 << UDRIE0);	/* enable usart data register empty interrupt */

		return TRUE;
	} else
		usart.TX_buffer.overflows++;
	return FALSE;
}

int8_t usart_receive_buffer(uint8_t size, uint8_t *buffer) {
	if (!size)
		return FALSE;

	if (usart.RX_buffer.length < size)
		return FALSE;

	uint8_t i;

	uint8_t sreg = SREG;		/* save state */
	cli();				/* disable interupts */

	for (i = 0; i < size; ++i) {
		buffer[i] = usart.RX_buffer.data[usart.RX_buffer.idx++];	/* copy from the front of the buf */
		if (usart.RX_buffer.idx >= BUFFER_SIZE)			/* increment index to front */
			usart.RX_buffer.idx -= BUFFER_SIZE;		/* wrap index */
		usart.RX_buffer.length--;					/* decrement length */
	}

	SREG = sreg;			/* restore interupt state */

	return TRUE;
}

ISR(USART_UDRE_vect) {
	if (usart.TX_buffer.length > 0) {	/* if there is data in the buffer */
		UDR0 = usart.TX_buffer.data[usart.TX_buffer.idx++];	/* populate the usart data reg */
		if (usart.TX_buffer.idx >= BUFFER_SIZE)		/* increment index to front of buf */
			usart.TX_buffer.idx -= BUFFER_SIZE;	/* wrap index */
		usart.TX_buffer.length--;				/* decrement length */
	} else {
		UCSR0B &= ~(1 << UDRIE0);	/* disable usart data register empty interrupt */
	}
}

ISR(USART_RX_vect) {
	uint8_t data;

	data = UDR0;

	if (usart.RX_buffer.length < BUFFER_SIZE) {
		usart.RX_buffer.data[(usart.RX_buffer.idx + usart.RX_buffer.length) % BUFFER_SIZE] = data;
		usart.RX_buffer.length++;
	} else {
		usart.RX_buffer.overflows++;
	}
}
