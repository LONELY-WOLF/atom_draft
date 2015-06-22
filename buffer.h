#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdint.h>

//Prototypes
extern uint8_t extract_u8(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len);
extern uint16_t extract_u16(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len);
extern uint32_t extract_u32(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len);
extern uint64_t extract_u56(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len);
extern int8_t extract_i8(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len);
extern int16_t extract_i16(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len);
extern int32_t extract_i32(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len);
extern int64_t extract_i56(uint16_t buf_off, uint32_t bit_off, uint8_t bit_len);
extern uint8_t getByte(uint16_t pos);
extern void addByte(uint8_t data);
extern void freeBytes(uint16_t count);
extern uint16_t getBufLen();

#endif