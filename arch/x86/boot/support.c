#include <stdint.h>

void *memset(void *dst, int value, __SIZE_TYPE__ size)
{
	uint8_t *out = (uint8_t *)dst;
	__SIZE_TYPE__ i;

	for (i = 0; i < size; ++i) {
		out[i] = (uint8_t)value;
	}

	return dst;
}
