#include "circbuf.h"

void circbuf_init(circbuf_t *cb) {
	uint8_t i;

	cb->idx = 0;
	cb->length = 0;
	cb->overflows = 0;

	for (i = 0; i < BUFFER_SIZE; ++i)
		cb->data[i] = 0;
}
