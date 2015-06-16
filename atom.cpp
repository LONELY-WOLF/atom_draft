// atom.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "atom.h"
#include "crc24q.h"

//Prototypes
void read();
int parsePacket();
uint8_t extract_u8(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len);
uint16_t extract_u16(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len);
uint32_t extract_u32(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len);
uint64_t extract_u56(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len);
int8_t extract_i8(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len);
int16_t extract_i16(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len);
int32_t extract_i32(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len);
int64_t extract_i56(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len);

uint8_t buffer[2048];
uint16_t buffer_p = 0;
uint16_t buffer_len = 0;

uint8_t bitmask[8] = { 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF };

#define CHECK_DATA(X) while(buffer_len < X) read();

FILE* input;

int main(int argc, char* argv[])
{
	input = fopen("input.bin", "rb");
	if (input > 0)
	{
		printf("file opened\n");
	}
	else
	{
		printf("can't open file\n");
		getchar();
		exit(1);
	}
	while (1)
	{
		parsePacket();
	}
}

int parsePacket()
{
	struct pvt_header mes_hdr;
	memset(&mes_hdr, 0, sizeof(mes_hdr));
	CHECK_DATA(5);
	while (buffer[buffer_p] != 0xD3)
	{
		read();
		buffer_p = (buffer_p + 1) & 0x3FF;
	}
	uint16_t id = extract_u16(buffer_p, 24, 12);
	//printf("ID = %d\n", extract_u16(&buffer[buffer_p], 24, 12));
	if (id == 4095) //Ashtech message
	{
		mes_hdr.length = extract_u16(buffer_p, 14, 10);
		printf("length: %d\n", mes_hdr.length);
		CHECK_DATA(mes_hdr.length + 6);
		mes_hdr.crc24 = buffer[(buffer_p + mes_hdr.length + 3) & 0x3FF];
		mes_hdr.crc24 <<= 8;
		mes_hdr.crc24 += buffer[(buffer_p + mes_hdr.length + 4) & 0x3FF];
		mes_hdr.crc24 <<= 8;
		mes_hdr.crc24 += buffer[(buffer_p + mes_hdr.length + 5) & 0x3FF];
		//printf("CRC = %X\n", mes_hdr.crc24);
		if (mes_hdr.crc24 != crc24q(0, buffer, buffer_p, mes_hdr.length + 3))
		{
			printf("CRC24Q failed: %X\n", crc24q(0, buffer, buffer_p, mes_hdr.length + 3));
			buffer_p = (buffer_p + 1) & 0x3FF;
			buffer_len--;
			return -1;
		}
		uint8_t message_sub_num = extract_u8(buffer_p, 36, 4);
		switch (message_sub_num)
		{
			case 3: //PVT
			{
				mes_hdr.version = extract_u8(buffer_p, 40, 3);
				mes_hdr.multi_mes = extract_u8(buffer_p, 43, 1);
				mes_hdr.nsats_used = extract_u8(buffer_p, 63, 6);
				mes_hdr.nsats_seen = extract_u8(buffer_p, 69, 6);
				mes_hdr.nsats_tracked = extract_u8(buffer_p, 75, 6);
				mes_hdr.pri_GNSS = extract_u8(buffer_p, 81, 2);
				printf("PVT: %d %d %d %d %d %d\n", mes_hdr.version, mes_hdr.multi_mes, mes_hdr.nsats_used, mes_hdr.nsats_seen, mes_hdr.nsats_tracked, mes_hdr.pri_GNSS);
				uint16_t block_p = buffer_p + 13;
				while (block_p < buffer_p + 13 + mes_hdr.length)
				{
					uint8_t block_len = extract_u8(block_p, 0, 8);
					uint8_t block_ID = extract_u8(block_p, 8, 4);
					switch (block_ID)
					{
						case 1:
						{
							//COO - Position
							if (block_len != 26)
							{
								printf("Wrong size of COO block\n");
								break;
							}
							struct coo_pvt_data coo_data;
							coo_data.pos_type = extract_u8(block_p, 12, 4);
							coo_data.GNSS_usage = extract_u8(block_p, 16, 8);
							coo_data.pos_mode = extract_u8(block_p, 24, 3);
							coo_data.pos_smoothing = extract_u8(block_p, 27, 3);
							coo_data.PDOP = extract_u16(block_p, 34, 10);
							coo_data.HDOP = extract_u16(block_p, 44, 10);
							coo_data.x = extract_i56(block_p, 54, 38);
							coo_data.y = extract_i56(block_p, 92, 38);
							coo_data.z = extract_i56(block_p, 130, 38);
							coo_data.diff_pos_age = extract_u16(block_p, 168, 10);
							coo_data.base_id = extract_u16(block_p, 178, 12);
							coo_data.pos_type_clarifier = extract_u8(block_p, 190, 4);
							coo_data.diff_link_age = extract_u16(block_p, 194, 10);
							printf("COO: %d %d %d %d\n", coo_data.pos_type, coo_data.x, coo_data.y, coo_data.z);
							break;
						}
						case 3:
						{
							//VEL - velosity
							if (block_len != 12)
							{
								printf("Wrong size of VEL block\n");
								break;
							}
							struct vel_pvt_data vel_data;
							vel_data.v1 = extract_i32(block_p, 12, 25);
							vel_data.v2 = extract_i32(block_p, 37, 25);
							vel_data.v3 = extract_i32(block_p, 62, 25);
							vel_data.vel_type = extract_u8(block_p, 87, 1);
							vel_data.vel_smoothing_int = extract_u8(block_p, 88, 4);
							vel_data.vel_frame = extract_u8(block_p, 92, 1);
							printf("VEL: %d %d %d %d\n", vel_data.v1, vel_data.v2, vel_data.v3, vel_data.vel_frame);
						}
						default:
						{
							printf("Unknown PVT block. ID = %d\n", block_ID);
							break;
						}
					}
					if (block_len > 0)
					{
						block_p += block_len;
					}
					else
					{
						block_p++; //In case of garbage
					}
				}
				break;
			}
		}
		buffer_p += mes_hdr.length + 6;
		buffer_p &= 0x3FF;
		buffer_len -= mes_hdr.length + 6;
	}
	else
	{
		buffer_p = (buffer_p + 1) & 0x3FF;
		buffer_len--;
	}
	return 0;
}

inline uint8_t extract_u8(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len)
{
	uint32_t offset = bit_off % 8;
	uint16_t retval = buffer[(buf_off + (bit_off >> 3)) & 0x3FF];
	retval <<= 8;
	retval += buffer[(buf_off + (bit_off >> 3) + 1) & 0x3FF];
	retval >>= (16 - bit_len - offset);
	retval &= bitmask[bit_len - 1];
	return (uint8_t)retval;
}

inline uint16_t extract_u16(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len)
{
	uint32_t offset = bit_off % 8;
	uint32_t retval = 0;
	for (int i = 0; i < 3; i++)
	{
		retval <<= 8;
		retval += buffer[(buf_off + (bit_off >> 3) + i) & 0x3FF];
	}
	retval = retval >> (24 - bit_len - offset);
	uint16_t mask = (uint16_t)bitmask[bit_len - 9];
	mask <<= 8;
	mask |= 0xFF;
	return ((uint16_t)retval) & mask;
}

inline uint32_t extract_u32(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len)
{
	uint32_t offset = bit_off % 8;
	uint64_t retval = 0;
	for (int i = 0; i < 5; i++)
	{
		retval <<= 8;
		retval += buffer[(buf_off + (bit_off >> 3) + i) & 0x3FF];
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

inline uint64_t extract_u56(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len)
{
	uint32_t offset = bit_off % 8;
	uint64_t retval = 0;
	for (int i = 0; i < 8; i++)
	{
		retval <<= 8;
		retval += buffer[(buf_off + (bit_off >> 3) + i) & 0x3FF];
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
	uint16_t retval = buffer[(buf_off + (bit_off >> 3)) & 0x3FF];
	retval <<= 8;
	retval += buffer[(buf_off + (bit_off >> 3) + 1) & 0x3FF];
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

inline int16_t extract_i16(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len)
{
	uint32_t offset = bit_off % 8;
	uint32_t retval = 0;
	for (int i = 0; i < 3; i++)
	{
		retval <<= 8;
		retval += buffer[(buf_off + (bit_off >> 3) + i) & 0x3FF];
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
	return *(int16_t*)&retval;; //LE only!
}

inline int32_t extract_i32(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len)
{
	uint32_t offset = bit_off % 8;
	uint64_t retval = 0;
	for (int i = 0; i < 5; i++)
	{
		retval <<= 8;
		retval += buffer[(buf_off + (bit_off >> 3) + i) & 0x3FF];
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
	return *(int32_t*)&retval;; //LE only!
}

//MADNESS #2!!!

inline int64_t extract_i56(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len)
{
	uint32_t offset = bit_off % 8;
	uint64_t retval = 0;
	for (int i = 0; i < 8; i++)
	{
		retval <<= 8;
		retval += buffer[(buf_off + (bit_off >> 3) + i) & 0x3FF];
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
	return *(int64_t*)&retval;; //LE only!
}

inline void read()
{
	int ch = 0;
	do
	{
		ch = fgetc(input);
	} while (errno != 0);
	if (ch == EOF)
	{
		printf("EOF!\n");
		getchar();
		exit(0);
	}
	buffer[(buffer_p + buffer_len) & 0x3FF] = ch;
	buffer_len++;
}
