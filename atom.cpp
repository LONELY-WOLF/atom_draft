#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "atom.h"
#include "crc24q.h"
#include "buffer.h"

uint16_t year_sun = 2012;

uint8_t days28[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
uint8_t days29[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

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
	struct hpr_pvt_data hpr_data;
	memset(&hpr_data, 0, sizeof(hpr_data));
	struct mis_pvt_data mis_data;
	memset(&mis_data, 0, sizeof(mis_data));
	struct time_tag_pvt_data time_tag_data;
	memset(&time_tag_data, 0, sizeof(time_tag_data));
	uint16_t id;
	uint8_t subid;
	uint8_t version;
	uint32_t crc24;
	uint8_t time_valid = 0;

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
				time_valid = 0;

				mes_hdr.multi_mes = extract_u8(0, 43, 1);
				mes_hdr.responseID = extract_u8(0, 54, 4);
				mes_hdr.nsats_used = extract_u8(0, 63, 6);
				mes_hdr.nsats_seen = extract_u8(0, 69, 6);
				mes_hdr.nsats_tracked = extract_u8(0, 75, 6);
				mes_hdr.pri_GNSS = extract_u8(0, 81, 2);
				time_tag_data.sec = extract_u16(0, 83, 12);
				time_tag_data.ext = extract_u8(0, 95, 1);
				if (time_tag_data.ext == 0)
				{
					time_tag_data.hour = extract_u8(0, 96, 5);
					time_tag_data.day = extract_u8(0, 101, 3);
				}
				printf("PVT: %d %d %d %d %d %d %d\n", version, mes_hdr.multi_mes, mes_hdr.nsats_used, mes_hdr.nsats_seen, mes_hdr.nsats_tracked, mes_hdr.pri_GNSS, mes_hdr.responseID);
				uint16_t block_p = 13;
				switch (mes_hdr.responseID)
				{
					//PVT
					case 0:
					{
						printf("PVT response\n");
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
									mis_data.GNSS_t_cycles = extract_u16(block_p, 88, 12);
									time_valid = 1;
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
								printf("VEL: %d %f %f %f %f\n", vel_data.vel_frame, v[0], v[1], v[2], -atan2(-v[1], v[0]) * 180.0f / 3.141592f);
							}
						}

						//Year
						if ((time_tag_data.ext == 0) && time_valid)
						{
							uint32_t days = (mis_data.GNSS_t_cycles * 7) + 6; //Days since 1 Jan 1980
							days -= 11688; //Days since 1 Jan 2012
							for (int i = 0; i < 80; i++)
							{
								if ((i & 3) == 0) //i % 4 == 0
								{
									if (days > 366)
									{
										days -= 366;
									}
									else
									{
										year_sun = 2012 + i;
										break;
									}
								}
								else
								{
									if (days > 365)
									{
										days -= 365;
									}
									else
									{
										year_sun = 2012 + i;
										break;
									}
								}
							}
							//Month and day
							uint8_t month_sun, day_sun;
							uint8_t *m_days = ((year_sun & 3) == 0) ? days29 : days28;
							for (int i = 0; i < 12; i++)
							{
								if (days > m_days[i])
								{
									days -= m_days[i];
								}
								else
								{
									month_sun = i + 1;
									day_sun = days;
									break;
								}
							}
							uint16_t year = year_sun;
							uint8_t month = month_sun;
							uint8_t day = day_sun + time_tag_data.day;
							if (day > m_days[month_sun])
							{
								day -= m_days[month_sun];
								month++;
								if (month > 12)
								{
									month -= 12;
									year++;
								}
							}
							uint8_t minutes = time_tag_data.sec / 60;
							uint8_t sec = time_tag_data.sec % 60;
							printf("Time: %02d:%02d:%02d %02d/%02d/%04d\n", time_tag_data.hour, minutes, sec, day, month, year);
						}

						break;
					}
					//ANG
					case 1:
					{
						printf("ANG response\n");
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
									printf("COO block\n");
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
									printf("ERR block\n");
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
									printf("VEL block\n");
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
									hpr_data.heading = extract_u16(block_p, 12, 16);
									hpr_data.pitch = extract_i16(block_p, 28, 16);
									hpr_data.roll = extract_i16(block_p, 44, 16);
									hpr_data.calib_mode = extract_u8(block_p, 60, 1);
									hpr_data.ambiguity = extract_u8(block_p, 61, 1);
									hpr_data.antenna_setup = extract_u8(block_p, 62, 2);
									hpr_data.MRMS = extract_u16(block_p, 64, 10);
									hpr_data.BRMS = extract_u16(block_p, 74, 10);
									hpr_data.platform_subID = extract_u8(block_p, 84, 4);
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
						float hpr[3];
						hpr[0] = hpr_data.heading / 100.0f;
						hpr[1] = hpr_data.pitch / 100.0f;
						hpr[2] = hpr_data.roll / 100.0f;
						if (hpr[0] > 360.0f)
						{
							hpr[0] = NAN;
						}
						if ((hpr[1] > 90.0f) || (hpr[1] < -90.0f))
						{
							hpr[1] = NAN;
						}
						if ((hpr[2] > 90.0f) || (hpr[2] < -90.0f))
						{
							hpr[2] = NAN;
						}
						printf("HPR: %.0f %.0f %.0f @ %d %d %d @ %d %d %d\n",
							hpr[0], hpr[1], hpr[2],
							hpr_data.calib_mode, hpr_data.ambiguity, hpr_data.antenna_setup,
							hpr_data.MRMS, hpr_data.BRMS, hpr_data.platform_subID);
						break;
					}
					default:
					{
						printf("Not supported response ID\n");
						break;
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
	float lat_f = lat / 10000000.0f * (3.141592653f / 180.0f);
	float lon_f = lon / 10000000.0f * (3.141592653f / 180.0f);
	float sin_lat = sinf(lat_f);
	float cos_lat = cosf(lat_f);
	float sin_lon = sinf(lon_f);
	float cos_lon = cosf(lon_f);

	v[0] = (-sin_lat * cos_lon * v_in[0]) + (-sin_lat * sin_lon * v_in[1]) + (cos_lat * v_in[2]);
	v[1] = (-sin_lon * v_in[0]) + (cos_lon * v_in[1]);
	v[2] = -((cos_lat * cos_lon * v_in[0]) + (cos_lat * sin_lon * v_in[1]) + (sin_lat * v_in[2]));
}