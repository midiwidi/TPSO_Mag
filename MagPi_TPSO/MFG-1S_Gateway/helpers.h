#ifndef HELPERS_H_
#define HELPERS_H_

#include <stdint.h>

#define NO_ERROR 0
#define ERROR 1
#define NO_DATA 0x7FFFFFFE
#define QUITE 0x7FFFFFFF

#define FALSE 0
#define TRUE 1

#define NO_FLUSH 0
#define FLUSH 1

#define DBG_DATA_OFF	0
#define DBG_BYTE_STREAM	0b00000001
#define DBG_PACKET		0b00000010
#define DBG_ALL			0b00000011

int limit_range(long *value, long llimit, long ulimit);
void clean_exit(int ret);
int32_t extend_sign_bit(int32_t value, uint8_t width);
void print_stream(uint8_t byte, uint8_t flush);
void print_cmd_usage(void);
void print_packet(uint8_t* packet, uint8_t packet_len);

#endif /* HELPERS_H_ */
