#ifndef ATOM_H_
#define ATOM_H_

#include <stdint.h>

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
uint8_t getByte(uint16_t pos);
void addByte(uint8_t data);

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
