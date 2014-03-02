#include "ring_buffer.h"

uint8_t rb_Put(RingBuffer_t *rb, char c)
{
	char *next = rb->Head + 1;
	if (next == rb->End)
		next = rb->Start;
	if (next == rb->Tail)
		return 0;
	*rb->Head = c;
	rb->Head = next;
	return 1;
}

uint8_t rb_Get(RingBuffer_t *rb, char * c)
{
	char *next;
	if (rb->Tail == rb->Head)	// No data in ring
	{
		*c = 0;
		return 0;
	}
	*c = *rb->Tail;				// Get data and move to next
	next = rb->Tail + 1;
	if (next == rb->End)		// Check for ring
		next = rb->Start;
	rb->Tail = next;
	return 1;
}

void rb_Init(RingBuffer_t *rb, char * buffer, int size)
{
	rb->Tail = rb->Head = rb->Start = buffer;
	rb->End = buffer + size;
}
