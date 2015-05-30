#ifndef ATOM_H_
#define ATOM_H_

#include <stdint.h>

struct pvt_header
{
	uint16_t length;
	uint8_t version;
	uint8_t multi_mes;
	uint8_t nsats_used;
	uint8_t nsats_seen;
	uint8_t nsats_tracked;
	uint8_t pri_GNSS;
	uint32_t time_tag;
	uint32_t crc24; //24bit
};

#endif /* ATOM_H_ */
