// atom.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "atom.h"
#include "crc24q.h"

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
	uint16_t id = extract_u16(0, 24, 12);
	//printf("ID = %d\n", extract_u16(&buffer[buffer_p], 24, 12));
	if (id == 4095) //Ashtech message
	{
		mes_hdr.length = extract_u16(0, 14, 10);
		printf("length: %d\n", mes_hdr.length);
		CHECK_DATA(mes_hdr.length + 6);
		mes_hdr.crc24 = getByte(mes_hdr.length + 3);
		mes_hdr.crc24 <<= 8;
		mes_hdr.crc24 += getByte(mes_hdr.length + 4);
		mes_hdr.crc24 <<= 8;
		mes_hdr.crc24 += getByte(mes_hdr.length + 5);
		//printf("CRC = %X\n", mes_hdr.crc24);
		if (mes_hdr.crc24 != crc24q(0, buffer, buffer_p, mes_hdr.length + 3))
		{
			printf("CRC24Q failed: %X\n", crc24q(0, buffer, buffer_p, mes_hdr.length + 3));
			buffer_p = (buffer_p + 1) & 0x3FF;
			buffer_len--;
			return -1;
		}
		uint8_t message_sub_num = extract_u8(0, 36, 4);
		switch (message_sub_num)
		{
			case 3: //PVT
			{
				mes_hdr.version = extract_u8(0, 40, 3);
				mes_hdr.multi_mes = extract_u8(0, 43, 1);
				mes_hdr.nsats_used = extract_u8(0, 63, 6);
				mes_hdr.nsats_seen = extract_u8(0, 69, 6);
				mes_hdr.nsats_tracked = extract_u8(0, 75, 6);
				mes_hdr.pri_GNSS = extract_u8(0, 81, 2);
				printf("PVT: %d %d %d %d %d %d\n", mes_hdr.version, mes_hdr.multi_mes, mes_hdr.nsats_used, mes_hdr.nsats_seen, mes_hdr.nsats_tracked, mes_hdr.pri_GNSS);
				uint16_t block_p = 13;
				while (block_p < 13 + mes_hdr.length)
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

							int32_t lat, lon, h;
							if ((coo_data.x == -137438953472) || (coo_data.y == -137438953472) || (coo_data.z == -137438953472))
							{
								printf("COO: invalid\n");
							}
							ecef2llh((double)coo_data.x / 10000.0, (double)coo_data.y / 10000.0, (double)coo_data.z / 10000.0, &lat, &lon, &h);
							printf("COO: %d %d %d %d\n", coo_data.pos_type, lat, lon, h);
							break;
						}
						case 2:
						{
							//ACC - Accuracy
							if (block_len != 10)
							{
								printf("Wrong size of ACC block\n");
								break;
							}
							uint32_t sigma = extract_u32(block_p, 12, 20);
							if (sigma == 1048575)
							{
								printf("ACC: invalid\n");
							}
							printf("ACC: %d\n", sigma);
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
							break;
						}
						case 14:
						{
							//SVS - Satellite Information
							uint8_t gnss_id = extract_u8(block_p, 12, 3);
							printf("SVS: size = %d, GNSS ID = %d\n", block_len, gnss_id);
							break;
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
	addByte(ch);
}

inline uint8_t getByte(uint16_t pos)
{
	return buffer[(buffer_p + pos) & 0x3FF];
}

inline void addByte(uint8_t data)
{
	buffer[(buffer_p + buffer_len) & 0x3FF] = data;
	buffer_len = (buffer_len + 1) & 0x3FF;
}

void ecef2llh(double x, double y, double z, int32_t* lat, int32_t* lon, int32_t* h)
{
	double he;
	double r_e = 6378137;          // WGS - 84 data
	double r_p = 6356752;
	double e = 0.08181979099211;
	double l = atan2(y, x);
	double eps = 1;
	double tol = 1e-8; //1e-7?
	double p = sqrt(x * x + y * y);
	double mu = atan(z / (p * (1 - e * e)));

	while (eps > tol)
	{
		double sinmu = sin(mu);
		double cosmu = cos(mu);
		double N = r_e * r_e / sqrt(r_e * r_e * cosmu * cosmu + r_p * r_p * sinmu * sinmu);
		he = p / cosmu - N;
		double mu0 = mu;
		mu = atan(z / (p * (1 - e * e * N / (N + he))));
		eps = abs(mu - mu0);
	}

	*lat = (int32_t)(mu * (180.0 / 3.141592653) * 10000000.0);
	*lon = (int32_t)(l * (180.0 / 3.141592653) * 10000000.0);
	*h = (int32_t)(he * 1000.0);
	return;
}
