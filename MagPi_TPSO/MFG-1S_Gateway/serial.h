#ifndef SERIAL_H_
#define SERIAL_H_

#include <stdint.h>
#include "packet.h"

#define SERIAL_POSTOPEN_DELAY 10000	//in microseconds
#define STREAM_BUFF_LEN	512
#define PACKET_BUFF_LEN (PACKET_LEN_MASSMEM+1) //1 extra byte for frame end

#define MAG_BAUDDIV_615384		1.0		// BaudDiv = 9.8304MHz / Baudrate / 16
#define MAG_BAUDDIV_115200		5.375

#define FEND		0xC0
#define FESC		0xDB
#define TFEND		0xDC
#define TFESC		0xDD

#define PACKET_NOT_COMPLETE 0

int open_serial(char *serial_port_device, uint32_t baud);
uint8_t slip_decode(uint8_t byte, uint8_t *packet);
void slip_encode(uint8_t byte_in, uint8_t **pp_buffer_wr);

#endif /* SERIAL_H_ */
