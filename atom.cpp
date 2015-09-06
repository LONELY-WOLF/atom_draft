#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "atom.h"
#include "crc24q.h"
#include "buffer.h"

#define CHECK_DATA(X) while(getBufLen() < X) read();
#define PX4_ARRAY2D(_array, _ncols, _x, _y) (_array[_x * _ncols + _y])
#define PX4_R(_array, _x, _y) PX4_ARRAY2D(_array, 3, _x, _y)

FILE* input;

int main(int argc, char* argv[])
{
	fopen_s(&input, "input.bin", "rb");
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
	struct coo_pvt_data coo_data;
	memset(&coo_data, 0, sizeof(coo_data));
	struct vel_pvt_data vel_data;
	memset(&vel_data, 0, sizeof(vel_data));
	struct err_pvt_data err_data;
	memset(&err_data, 0, sizeof(err_data));
	uint16_t id;
	uint8_t subid;
	uint8_t version;
	uint32_t crc24;

	if (check_data(5) != 0)
	{
		return -1;
	}
	while (getByte(0) != 0xD3)
	{
		if (read(1) != 0)
		{
			return -1;
		}
		freeBytes(1);
	}
	id = extract_u16(0, 24, 12);
	//printf("ID = %d", id);
	//Ashtech message
	if (id == 4095)
	{
		uint16_t mes_length = extract_u16(0, 14, 10);
		printf("length: %d\n", mes_length);
		if (check_data(mes_length + 6) != 0)
		{
			return -1;
		}
		crc24 = getByte(mes_length + 3);
		crc24 <<= 8;
		crc24 += getByte(mes_length + 4);
		crc24 <<= 8;
		crc24 += getByte(mes_length + 5);
		//printf("CRC = %X\n", mes_hdr.crc24);
		if (crc24 != crc24q(mes_length + 3))
		{
			printf("CRC24Q failed: %X\n", crc24q(mes_length + 3));
			freeBytes(1);
			return -1;
		}
		subid = extract_u8(0, 36, 4);
		version = extract_u8(0, 40, 3);
		switch (subid)
		{
			//PVT
			case 3:
			{
				//Make init data invalid
				coo_data.x = -137438953472;
				vel_data.v1 = -16777216;
				err_data.sigma = 1048575;

				mes_hdr.multi_mes = extract_u8(0, 43, 1);
				mes_hdr.nsats_used = extract_u8(0, 63, 6);
				mes_hdr.nsats_seen = extract_u8(0, 69, 6);
				mes_hdr.nsats_tracked = extract_u8(0, 75, 6);
				mes_hdr.pri_GNSS = extract_u8(0, 81, 2);
				printf("PVT: %d %d %d %d %d %d\n", version, mes_hdr.multi_mes, mes_hdr.nsats_used, mes_hdr.nsats_seen, mes_hdr.nsats_tracked, mes_hdr.pri_GNSS);
				uint16_t block_p = 13;
				if (mes_hdr.responseID == 0)
				{
					while (block_p < 3 + mes_length - 2)
					{
						//printf("at %d:\n", block_p);
						uint8_t block_len = extract_u8(block_p, 0, 8);
						uint8_t block_ID = extract_u8(block_p, 8, 4);
						switch (block_ID)
						{
							//COO - position
							case 1:
							{
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
							//ERR - accuracy
							case 2:
							{
								if (block_len != 10)
								{
									printf("Wrong size of ERR block\n");
									break;
								}
								err_data.sigma = extract_u32(block_p, 12, 20);
								err_data.k1 = extract_u8(block_p, 32, 7);
								err_data.k2 = extract_u8(block_p, 39, 7);
								err_data.k3 = extract_u8(block_p, 46, 7);
								err_data.r12 = extract_i8(block_p, 53, 8);
								err_data.r13 = extract_i8(block_p, 61, 8);
								err_data.r23 = extract_i8(block_p, 69, 8);
								if (err_data.sigma == 1048575)
								{
									printf("ERR: invalid\n");
								}
								break;
							}
							//VEL - velocity
							case 3:
							{
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
							//CLK - clock
							case 4:
							{
								if (block_len != 10)
								{
									printf("Wrong size of CLK block\n");
									break;
								}
								printf("CLK block\n");
								break;
							}
							//LCY - latency
							case 5:
							{
								if (block_len != 3)
								{
									printf("Wrong size of LCY block\n");
									break;
								}
								printf("LCY block\n");
								break;
							}
							//HPR - attitude
							case 6:
							{
								if (block_len != 11)
								{
									printf("Wrong size of HPR block\n");
									break;
								}
								printf("HPR block\n");
								break;
							}
							//BLN - baseline
							case 7:
							{
								if (block_len != 16)
								{
									printf("Wrong size of BLN block\n");
									break;
								}
								printf("BLN block\n");
								break;
							}
							//MIS - miscellaneous
							case 8:
							{
								if (block_len != 23)
								{
									printf("Wrong size of MIS block\n");
									break;
								}
								printf("MIS block\n");
								break;
							}
							//ROT - extended attitude parameters
							case 9:
							{
								if (block_len != 13)
								{
									printf("Wrong size of ROT block\n");
									break;
								}
								printf("ROT block\n");
								break;
							}
							//BSD - extended baseline parameters
							case 10:
							{
								if (block_len != 19)
								{
									printf("Wrong size of BSD block\n");
									break;
								}
								printf("BSD block\n");
								break;
							}
							//ARR - arrow (vectors of platforms)
							case 11:
							{
								if (block_len != 17)
								{
									printf("Wrong size of ARR block\n");
									break;
								}
								printf("ARR block\n");
								break;
							}
							//ASD - extended arrow parameters
							case 12:
							{
								if (block_len != 19)
								{
									printf("Wrong size of ASD block\n");
									break;
								}
								printf("ASD block\n");
								break;
							}
							//SVS - Satellite Information
							case 14:
							{
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
						freeBytes(mes_length + 6); //free header + message + crc24q
						return 1;
					}
					else
					{
						int32_t lat, lon, alt;
						ecef2llh(&coo_data, &lat, &lon, &alt);
						printf("COO: %d %d %d %d\n", coo_data.pos_type ? coo_data.pos_type + 2 : 0, lat, lon, alt);

						//Accuracy
						if (err_data.sigma == 1048575)
						{
							printf("ERR: invalid\n");
						}
						else
						{
							float eph = sqrtf((float)err_data.k1 * err_data.k1 + err_data.k2 * err_data.k2) * err_data.sigma / (1000.0f * 128.0f);
							float epv = err_data.k3 * err_data.sigma / (1000.0f * 128.0f);
							printf("ERR: %f %f\n", eph, epv);
						}

						//Velocity
						if ((vel_data.v1 == -16777216) || (vel_data.v2 == -16777216) || (vel_data.v3 == -16777216))
						{
							printf("VEL: invalid\n");
						}
						else
						{
							float v[3];
							xyz2ned(&vel_data, lat, lon, v);
							//ecef2ned(vel_data.v1, vel_data.v2, vel_data.v2, coo_data.x, coo_data.y, coo_data.z, &vn, &ve, &vd);
							printf("VEL: %d %f %f %f %f\n", vel_data.vel_frame, v[0], v[1], v[2], atan2(v[0], v[1]) * 180.0f / 3.141592f);
						}
					}
				}
				else
				{
					printf("Not supported response ID\n");
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
	return 1;
}

int check_data(uint16_t size)
{
	while (size > getBufLen())
	{
		read(size - getBufLen());
	}
	return 0;
}

int read(uint16_t count)
{
	uint8_t buf[32];
	uint16_t left = count;
	while (left > 0)
	{
		int bytes_read = fread(buf, 1, (left > 32) ? 32 : left, input);
		if (bytes_read < 0)
		{
			return bytes_read;
		}
		else
		{
			if (bytes_read)
			{
				left -= bytes_read;
				for (int i = 0; i < bytes_read; i++)
				{
					addByte(buf[i]); //make addBytes() function
				}
			}
			else
			{
				printf("EOF\n");
				//getchar();
				exit(0);
			}
		}
	}
	return 0;
}

void ecef2llh(struct coo_pvt_data* coo, int32_t* lat, int32_t* lon, int32_t* h)
{
	double x_d = coo->x / 10000.0;
	double y_d = coo->y / 10000.0;
	double z_d = coo->z / 10000.0;
	double he = 0.0;
	const double r_e2 = 6378137.0 * 6378137.0;          // WGS - 84 data
	const double r_p2 = 6356752.0 * 6356752.0;
	const double e2 = 0.08181979099211 * 0.08181979099211;
	double l = atan2(y_d, x_d);
	double eps = 1.0;
	const double tol = 1.0e-8; //1.0e-7?
	double p = sqrt(x_d * x_d + y_d * y_d);
	double mu = atan(z_d / (p * (1.0 - e2)));

	//TODO: replace with more efficient method
	while (eps > tol) // 1 time for 1.0e-7, 2 times for 1.0e-8
	{
		double sinmu = sin(mu);
		double cosmu = cos(mu);
		double N = r_e2 / sqrt(r_e2 * cosmu * cosmu + r_p2 * sinmu * sinmu);
		he = p / cosmu - N;
		double mu0 = mu;
		mu = atan(z_d / (p * (1.0 - e2 * N / (N + he))));
		eps = fabs(mu - mu0);
	}

	*lat = (int32_t)(mu * (180.0 / 3.141592653) * 10000000.0);
	*lon = (int32_t)(l * (180.0 / 3.141592653) * 10000000.0);
	*h = (int32_t)(he * 1000.0);
	return;
}

void xyz2ned(struct vel_pvt_data* vel, int32_t lat, int32_t lon, float v[3])
{
	float v_in[3] = { vel->v1 / 10000.0f, vel->v2 / 10000.0f, vel->v3 / 10000.0f };
	float enu[3];
	float R[9];
	float lat_f = lat / 10000000.0f;
	float lon_f = lon / 10000000.0f;
	float sin_lat = sinf(lat_f);
	float cos_lat = cosf(lat_f);
	float sin_lon = sinf(lon_f);
	float cos_lon = cosf(lon_f);
	PX4_R(R, 0, 0) = -sin_lon;
	PX4_R(R, 0, 1) = cos_lon;
	PX4_R(R, 0, 2) = 0.0f;
	PX4_R(R, 1, 0) = -sin_lat * cos_lon;
	PX4_R(R, 1, 1) = -sin_lat * sin_lon;
	PX4_R(R, 1, 2) = cos_lat;
	PX4_R(R, 2, 0) = cos_lat * cos_lon;
	PX4_R(R, 2, 1) = cos_lat * sin_lon;
	PX4_R(R, 2, 2) = sin_lat;

	for (int i = 0; i < 3; i++)
	{
		enu[i] = 0.0f;

		for (int j = 0; j < 3; j++)
		{
			enu[i] += PX4_R(R, j, i) * v_in[j];
		}
	}

	v[0] = enu[1]; //N
	v[1] = enu[0]; //E
	v[2] = -enu[2]; //D
}

//void ecef2ned(int32_t v_x, int32_t v_y, int32_t v_z, int64_t ref_x, int64_t ref_y, int64_t ref_z, float* v_n, float* v_e, float* v_d)
//{
//	//int32_t for ref?
//	float ref_xf = ref_x / 10000.0f;
//	float ref_yf = ref_y / 10000.0f;
//	float ref_zf = ref_z / 10000.0f;
//	float xyz[3] = { v_x / 10000.0f, v_y / 10000.0f, v_z / 10000.0f };
//	float ned[3] = { 0.0f, 0.0f, 0.0f };
//
//	float hyp_az, hyp_el;
//	float sin_el, cos_el, sin_az, cos_az;
//
//	hyp_az = sqrt(ref_xf * ref_xf + ref_yf * ref_yf);
//	hyp_el = sqrt(hyp_az * hyp_az + ref_zf * ref_zf);
//	sin_el = ref_zf / hyp_el;
//	cos_el = hyp_az / hyp_el;
//	sin_az = ref_yf / hyp_az;
//	cos_az = ref_xf / hyp_az;
//
//	float M[3][3];
//	M[0][0] = -sin_el * cos_az;
//	M[0][1] = -sin_el * sin_az;
//	M[0][2] = cos_el;
//	M[1][0] = -sin_az;
//	M[1][1] = cos_az;
//	M[1][2] = 0.0;
//	M[2][0] = -cos_el * cos_az;
//	M[2][1] = -cos_el * sin_az;
//	M[2][2] = -sin_el;
//
//	uint8_t i, j, k;
//	for (i = 0; i < 3; i++)
//	{
//		for (j = 0; j < 1; j++) {
//			ned[1 * i + j] = 0;
//			for (k = 0; k < 3; k++)
//			{
//				ned[1 * i + j] += M[i][k] * xyz[1 * k + j];
//			}
//		}
//	}
//
//	*v_n = ned[0];
//	*v_e = ned[1];
//	*v_d = ned[2];
//	return;
//}