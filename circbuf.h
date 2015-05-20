#ifndef _CIRCBUF_H
#define _CIRCBUF_H

#include <inttypes.h>

#define BUFFER_SIZE 64

typedef struct circular_buffer {
	uint8_t	idx;
	uint8_t	length;
	uint16_t overflows;
	uint8_t data[BUFFER_SIZE];
} circbuf_t;

void circbuf_init(circbuf_t *cb);

#endif /* _CIRCBUF_H */
