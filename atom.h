#ifndef ATOM_H_
#define ATOM_H_

#include <stdint.h>

//Prototypes
extern void read();
extern int parsePacket();
extern void ecef2llh(double x, double y, double z, int32_t* lat, int32_t* lon, int32_t* h);

struct pvt_header
{
	uint8_t version;
	uint8_t multi_mes;
	uint8_t nsats_used;
	uint8_t nsats_seen;
	uint8_t nsats_tracked;
	uint8_t pri_GNSS;
	uint32_t time_tag;
	uint32_t crc24;
};

struct coo_pvt_data
{
	uint8_t pos_type;
	uint8_t GNSS_usage;
	uint8_t pos_mode;
	uint8_t pos_smoothing;
	uint16_t PDOP;
	uint16_t HDOP;
	int64_t x, y, z;
	uint16_t diff_pos_age;
	uint16_t base_id;
	uint8_t pos_type_clarifier;
	uint16_t diff_link_age;
};

struct vel_pvt_data
{
	int32_t v1, v2, v3;
	uint8_t vel_type;
	uint8_t vel_smoothing_int;
	uint8_t vel_frame;
};

#endif /* ATOM_H_ */
