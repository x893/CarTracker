#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#include <stdint.h>

typedef struct RingBuffer_s {
	char * Head;
	char * Tail;
	char * Start;
	char * End;
} RingBuffer_t;

uint8_t rb_Get(RingBuffer_t *rb, char * c);
uint8_t rb_Put(RingBuffer_t *rb, char c);
void    rb_Init(RingBuffer_t *rb, char * buffer, int size);

#endif
