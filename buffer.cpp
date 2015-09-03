#include <stdio.h>
#include "buffer.h"

uint8_t buffer[2048];
uint16_t buffer_p = 0;
uint16_t buffer_len = 0;

uint8_t bitmask[8] = { 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF };

inline uint8_t extract_u8(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len)
{
	uint32_t offset = bit_off % 8;
	uint16_t retval = getByte(buf_off + (bit_off >> 3));
	retval <<= 8;
	retval += getByte(buf_off + (bit_off >> 3) + 1);
	retval >>= (16 - bit_len - offset);
	retval &= bitmask[bit_len - 1];
	return (uint8_t)retval;
}

uint16_t extract_u16(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len)
{
	uint32_t offset = bit_off % 8;
	uint32_t retval = 0;
	for (int i = 0; i < 3; i++)
	{
		retval <<= 8;
		retval += getByte(buf_off + (bit_off >> 3) + i);
	}
	retval = retval >> (24 - bit_len - offset);
	uint16_t mask = (uint16_t)bitmask[bit_len - 9];
	mask <<= 8;
	mask |= 0xFF;
	return ((uint16_t)retval) & mask;
}

uint32_t extract_u32(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len)
{
	uint32_t offset = bit_off % 8;
	uint64_t retval = 0;
	for (int i = 0; i < 5; i++)
	{
		retval <<= 8;
		retval += getByte(buf_off + (bit_off >> 3) + i);
	}
	retval = retval >> (40 - bit_len - offset);
	uint32_t mask = 1;
	for (uint8_t i = 1; i < bit_len; i++)
	{
		mask <<= 1;
		mask++;
	}
	return ((uint32_t)retval) & mask;
}

//MADNESS!!!

uint64_t extract_u56(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len)
{
	uint32_t offset = bit_off % 8;
	uint64_t retval = 0;
	for (int i = 0; i < 8; i++)
	{
		retval <<= 8;
		retval += getByte(buf_off + (bit_off >> 3) + i);
	}
	retval = retval >> (64 - bit_len - offset);
	uint64_t mask = 1;
	for (uint8_t i = 1; i < bit_len; i++)
	{
		mask <<= 1;
		mask++;
	}
	return retval & mask;
}

inline int8_t extract_i8(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len)
{
	uint32_t offset = bit_off % 8;
	uint16_t retval = getByte(buf_off + (bit_off >> 3));
	retval <<= 8;
	retval += getByte(buf_off + (bit_off >> 3) + 1);
	retval >>= (16 - bit_len - offset);
	if ((retval >> (bit_len - 1)) & 1) //Negative
	{
		retval |= ~bitmask[bit_len - 1];
	}
	else
	{
		retval &= bitmask[bit_len - 1];
	}
	return *(int8_t*)&retval; //LE only!
}

int16_t extract_i16(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len)
{
	uint32_t offset = bit_off % 8;
	uint32_t retval = 0;
	for (int i = 0; i < 3; i++)
	{
		retval <<= 8;
		retval += getByte(buf_off + (bit_off >> 3) + i);
	}
	retval = retval >> (24 - bit_len - offset);
	uint16_t mask = (uint16_t)bitmask[bit_len - 9];
	mask <<= 8;
	mask |= 0xFF;
	if ((retval >> (bit_len - 1)) & 1) //Negative
	{
		retval |= ~mask;
	}
	else
	{
		retval &= mask;
	}
	return *(int16_t*)&retval; //LE only!
}

int32_t extract_i32(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len)
{
	uint32_t offset = bit_off % 8;
	uint64_t retval = 0;
	for (int i = 0; i < 5; i++)
	{
		retval <<= 8;
		retval += getByte(buf_off + (bit_off >> 3) + i);
	}
	retval = retval >> (40 - bit_len - offset);
	uint32_t mask = 1;
	for (uint8_t i = 1; i < bit_len; i++)
	{
		mask <<= 1;
		mask++;
	}
	if ((retval >> (bit_len - 1)) & 1) //Negative
	{
		retval |= ~mask;
	}
	else
	{
		retval &= mask;
	}
	return *(int32_t*)&retval; //LE only!
}

//MADNESS #2!!!

int64_t extract_i56(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len)
{
	uint32_t offset = bit_off % 8;
	uint64_t retval = 0;
	for (int i = 0; i < 8; i++)
	{
		retval <<= 8;
		retval += getByte(buf_off + (bit_off >> 3) + i);
	}
	retval = retval >> (64 - bit_len - offset);
	uint64_t mask = 1;
	for (uint8_t i = 1; i < bit_len; i++)
	{
		mask <<= 1;
		mask++;
	}

	if ((retval >> (bit_len - 1)) & 1) //Negative
	{
		retval |= ~mask;
	}
	else
	{
		retval &= mask;
	}
	return *(int64_t*)&retval; //LE only!
}

inline uint8_t getByte(uint16_t pos)
{
	if (pos <= buffer_len)
	{
		return buffer[(buffer_p + pos) & 0x3FF];
	}
	else
	{
		printf("Out of bounds!\n");
		return 0;
	}
}

inline void addByte(uint8_t data)
{
	if (buffer_len < 2048)
	{
		buffer[(buffer_p + buffer_len) & 0x3FF] = data;
		buffer_len = (buffer_len + 1) & 0x3FF;
	}
	else
	{
		printf("Buffer overflow!\n");
	}
}

inline void freeBytes(uint16_t count)
{
	buffer_p = (buffer_p + count) & 0x3FF;
	buffer_len = (buffer_len - count) & 0x3FF;
	if (buffer_len < 0)
	{
		printf("Freeing more bytes then buffer has!\n");
		buffer_len = 0;
	}
}

inline uint16_t getBufLen()
{
	return buffer_len;
}