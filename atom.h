#ifndef ATOM_H_
#define ATOM_H_

#include <stdint.h>

//Prototypes
extern int read(uint16_t count);
extern int check_data(uint16_t size);
extern int parsePacket();
extern void ecef2llh(struct coo_pvt_data* coo, int32_t* lat, int32_t* lon, int32_t* h);
void xyz2ned(struct vel_pvt_data* vel, int32_t lat, int32_t lon, float v[3]);
void ecef2ned(struct vel_pvt_data* vel, struct coo_pvt_data* coo, float v[3]);

struct pvt_header
{
	uint8_t multi_mes;
	uint8_t antennaID;
	uint8_t engineID;
	uint8_t responseID;
	uint8_t nsats_used;
	uint8_t nsats_seen;
	uint8_t nsats_tracked;
	uint8_t pri_GNSS;
	uint32_t time_tag;
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

struct err_pvt_data
{
	uint32_t sigma;
	uint8_t k1, k2, k3;
	int8_t r12, r13, r23;
};
#endif /* ATOM_H_ */
