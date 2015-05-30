// atom.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "atom.h"
#include "crc24.h"

//Prototypes
void read();
int parsePacket();
uint8_t extract_u8(uint8_t* data, uint32_t bit_off, uint8_t bit_len);
uint16_t extract_u16(uint8_t* data, uint8_t bit_off, uint8_t bit_len);

uint8_t buffer[256];
uint8_t buffer_p = 0;
uint8_t buffer_len = 0;

uint8_t bitmask[8] = { 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF };

#define CHECK_DATA(X) while(buffer_len < X) read();

FILE* input;

int _tmain(int argc, _TCHAR* argv[])
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
		buffer_p = (buffer_p + 1) & 0xFF;
	}
	printf("ID = %d\n", extract_u16(&buffer[buffer_p], 24, 12));
	if (extract_u16(&buffer[buffer_p], 24, 12) == 4095) //Ashtech message
	{
		printf("Ashtech\n");
		mes_hdr.length = extract_u16(&buffer[buffer_p], 14, 10);
		printf("length: %d\n", mes_hdr.length);
		CHECK_DATA(mes_hdr.length + 6);
		mes_hdr.crc24 = buffer[(buffer_p + mes_hdr.length + 3) && 0xFF];
		mes_hdr.crc24 <<= 8;
		mes_hdr.crc24 = buffer[(buffer_p + mes_hdr.length + 4) && 0xFF];
		mes_hdr.crc24 <<= 8;
		mes_hdr.crc24 = buffer[(buffer_p + mes_hdr.length + 5) && 0xFF];
		printf("CRC = %X\n", mes_hdr.crc24);
		if (mes_hdr.crc24 != crc24_calc(INIT_CRC24, &buffer[(buffer_p + 3) && 0xFF], mes_hdr.length))
		{
			printf("CRC failed\n");
		}
		else
		{
			buffer_p += mes_hdr.length + 6;
			buffer_len -= mes_hdr.length + 6;
			printf("CRC OK!\n");
		}
		/*uint8_t message_sub_num = extract_u8(&buffer[buffer_p], 36, 4);
		switch(message_sub_num)
		{
		case 3: //PVT
		{
		mes_hdr.version = extract_u8(&buffer[buffer_p], 40, 3);
		mes_hdr.multi_mes = extract_u8(&buffer[buffer_p], 43, 1);
		mes_hdr.nsats_used = extract_u8(&buffer[buffer_p], 63, 6);
		mes_hdr.nsats_seen = extract_u8(&buffer[buffer_p], 69, 6);
		mes_hdr.nsats_tracked = extract_u8(&buffer[buffer_p], 75, 6);
		mes_hdr.pri_GNSS = extract_u8(&buffer[buffer_p], 81, 2);
		break;
		}
		}*/
	}
	else
	{
		buffer_p = (buffer_p + 1) & 0xFF;
		buffer_len--;
	}
	return 0;
}

inline uint8_t extract_u8(uint8_t* data, uint32_t bit_off, uint8_t bit_len)
{
	uint32_t offset = bit_off % 8;
	uint16_t retval = *((uint16_t*)(&data[bit_off / 8]));
	retval = retval >> (16 - bit_len - offset);
	return (uint8_t)(retval & bitmask[bit_len - 1]);
}

inline uint16_t extract_u16(uint8_t* data, uint8_t bit_off, uint8_t bit_len)
{
	uint32_t offset = bit_off % 8;
	uint32_t retval = *((uint32_t*)(&data[bit_off / 8]));
	retval = retval >> (32 - bit_len - offset);
	return (uint16_t)(retval & (((uint16_t)bitmask[bit_len - 9] << 8) | 0xFF));
}

void read()
{
	buffer_len++;
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
	buffer[(buffer_p + buffer_len) & 0xFF] = ch;
}
