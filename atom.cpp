#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "atom.h"
#include "crc24q.h"
#include "buffer.h"

#define CHECK_DATA(X) while(getBufLen() < X) read();

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
	struct coo_pvt_data coo_data;
	struct vel_pvt_data vel_data;
	memset(&mes_hdr, 0, sizeof(mes_hdr));
	CHECK_DATA(5);
	while (getByte(0) != 0xD3)
	{
		read();
		freeBytes(1);
	}
	uint16_t id = extract_u16(0, 24, 12);
	//printf("ID = %d\n", extract_u16(&buffer[buffer_p], 24, 12));
	if (id == 4095) //Ashtech message
	{
		uint16_t mes_length = extract_u16(0, 14, 10);
		printf("length: %d\n", mes_length);
		CHECK_DATA(mes_length + 6);
		mes_hdr.crc24 = getByte(mes_length + 3);
		mes_hdr.crc24 <<= 8;
		mes_hdr.crc24 += getByte(mes_length + 4);
		mes_hdr.crc24 <<= 8;
		mes_hdr.crc24 += getByte(mes_length + 5);
		//printf("CRC = %X\n", mes_hdr.crc24);
		if (mes_hdr.crc24 != crc24q(mes_length + 3))
		{
			printf("CRC24Q failed: %X\n", crc24q(mes_length + 3));
			freeBytes(1);
			return -1;
		}
		uint8_t message_sub_num = extract_u8(0, 36, 4);
		switch (message_sub_num)
		{
			case 3: //PVT
			{
				//Make init data invalid
				coo_data.x = -137438953472;
				vel_data.v1 = -16777216;

				mes_hdr.version = extract_u8(0, 40, 3);
				mes_hdr.multi_mes = extract_u8(0, 43, 1);
				mes_hdr.nsats_used = extract_u8(0, 63, 6);
				mes_hdr.nsats_seen = extract_u8(0, 69, 6);
				mes_hdr.nsats_tracked = extract_u8(0, 75, 6);
				mes_hdr.pri_GNSS = extract_u8(0, 81, 2);
				printf("PVT: %d %d %d %d %d %d\n", mes_hdr.version, mes_hdr.multi_mes, mes_hdr.nsats_used, mes_hdr.nsats_seen, mes_hdr.nsats_tracked, mes_hdr.pri_GNSS);
				uint16_t block_p = 13;
				while (block_p < 13 + mes_length)
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
							break;
						}
						case 2:
						{
							//ERR - Accuracy
							if (block_len != 10)
							{
								printf("Wrong size of ERR block\n");
								break;
							}
							uint32_t sigma = extract_u32(block_p, 12, 20);
							if (sigma == 1048575)
							{
								printf("ERR: invalid\n");
							}
							printf("ERR: %d\n", sigma);
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
							vel_data.v1 = extract_i32(block_p, 12, 25);
							vel_data.v2 = extract_i32(block_p, 37, 25);
							vel_data.v3 = extract_i32(block_p, 62, 25);
							vel_data.vel_type = extract_u8(block_p, 87, 1);
							vel_data.vel_smoothing_int = extract_u8(block_p, 88, 4);
							vel_data.vel_frame = extract_u8(block_p, 92, 1);
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
				//Convert data
				if ((coo_data.x == -137438953472) || (coo_data.y == -137438953472) || (coo_data.z == -137438953472))
				{
					printf("COO: invalid\n");
				}
				else
				{
					int32_t lat, lon, h;
					ecef2llh(coo_data.x / 10000.0, coo_data.y / 10000.0, coo_data.z / 10000.0, &lat, &lon, &h);
					printf("COO: %d %d %d %d\n", coo_data.pos_type, lat, lon, h);

					//Velocity
					if ((vel_data.v1 == -16777216) || (vel_data.v2 == -16777216) || (vel_data.v3 == -16777216))
					{
						printf("VEL: invalid\n");
					}
					else
					{
						float vn, ve, vd;
						ecef2ned(vel_data.v1, vel_data.v2, vel_data.v2, coo_data.x, coo_data.y, coo_data.z, &vn, &ve, &vd);
						printf("VEL: %d %lf %lf %lf\n", vel_data.vel_frame, vn, ve, vd);
					}
				}
				break;
			}
		}
		freeBytes(mes_length + 6); //free header + message + crc24q
	}
	else
	{
		freeBytes(1);
	}
	return 0;
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

void ecef2llh(double x, double y, double z, int32_t* lat, int32_t* lon, int32_t* h)
{
	double he = 0.0;
	const double r_e2 = 6378137.0 * 6378137.0;          // WGS - 84 data
	const double r_p2 = 6356752.0 * 6356752.0;
	const double e2 = 0.08181979099211 * 0.08181979099211;
	double l = atan2(y, x);
	double eps = 1.0;
	const double tol = 1.0e-8; //1.0e-7?
	double p = sqrt(x * x + y * y);
	double mu = atan(z / (p * (1.0 - e2)));

	//TODO: replace with more efficient method
	while (eps > tol)
	{
		double sinmu = sin(mu);
		double cosmu = cos(mu);
		double N = r_e2 / sqrt(r_e2 * cosmu * cosmu + r_p2 * sinmu * sinmu);
		he = p / cosmu - N;
		double mu0 = mu;
		mu = atan(z / (p * (1.0 - e2 * N / (N + he))));
		eps = abs(mu - mu0);
	}

	*lat = (int32_t)(mu * (180.0 / 3.141592653) * 10000000.0);
	*lon = (int32_t)(l * (180.0 / 3.141592653) * 10000000.0);
	*h = (int32_t)(he * 1000.0);
	return;
}

void ecef2ned(int32_t v_x, int32_t v_y, int32_t v_z, int64_t ref_x, int64_t ref_y, int64_t ref_z, float* v_n, float* v_e, float* v_d)
{
	//int32_t for ref?
	float ref_xf = ref_x / 10000.0f;
	float ref_yf = ref_y / 10000.0f;
	float ref_zf = ref_z / 10000.0f;
	float xyz[3] = { v_x / 10000.0f, v_y / 10000.0f, v_z / 10000.0f };
	float ned[3] = { 0.0f, 0.0f, 0.0f };

	float hyp_az, hyp_el;
	float sin_el, cos_el, sin_az, cos_az;

	hyp_az = sqrt(ref_xf * ref_xf + ref_yf * ref_yf);
	hyp_el = sqrt(hyp_az * hyp_az + ref_zf * ref_zf);
	sin_el = ref_zf / hyp_el;
	cos_el = hyp_az / hyp_el;
	sin_az = ref_yf / hyp_az;
	cos_az = ref_xf / hyp_az;

	float M[3][3];
	M[0][0] = -sin_el * cos_az;
	M[0][1] = -sin_el * sin_az;
	M[0][2] = cos_el;
	M[1][0] = -sin_az;
	M[1][1] = cos_az;
	M[1][2] = 0.0;
	M[2][0] = -cos_el * cos_az;
	M[2][1] = -cos_el * sin_az;
	M[2][2] = -sin_el;

	uint8_t i, j, k;
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 1; j++) {
			ned[1 * i + j] = 0;
			for (k = 0; k < 3; k++)
			{
				ned[1 * i + j] += M[i][k] * xyz[1 * k + j];
			}
		}
	}

	*v_n = ned[0];
	*v_e = ned[1];
	*v_d = ned[2];
	return;
}