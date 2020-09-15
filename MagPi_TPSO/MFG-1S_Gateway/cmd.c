#include <stdio.h>
#include <unistd.h>

#include "cmd.h"
#include "globals.h"
#include "helpers.h"
#include "serial.h"
#include "log.h"

const cmd_id_t cmd =
{
	 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
	 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37
};

int send_cmd(uint8_t cmd_id, uint32_t cmd_data)
{
	ssize_t bytes_written;
	size_t bytes_to_write;
	uint8_t cmd_buff[2*CMD_LENGTH+1];	// In a worst case (each byte would be escaped) the number of transmitted bytes
										// doubles. In addition we need 1 byte for FEND
	uint8_t *p_cmd_buff = cmd_buff;
	uint16_t crc;

	slip_encode(cmd_id, &p_cmd_buff);
	slip_encode((uint8_t) (cmd_data & 0xFF), &p_cmd_buff);
	slip_encode((uint8_t) ((cmd_data >> 8) & 0xFF), &p_cmd_buff);
	slip_encode((uint8_t) ((cmd_data >> 16) & 0xFF), &p_cmd_buff);
	crc = CheckCRC16(cmd_buff, CMD_LENGTH - 2);
	slip_encode((uint8_t) (crc & 0xFF), &p_cmd_buff);
	slip_encode((uint8_t) ((crc >> 8) & 0xFF), &p_cmd_buff);
	*p_cmd_buff = FEND;
	p_cmd_buff++;

	bytes_to_write = p_cmd_buff-cmd_buff;
	bytes_written = write(g_ser_port, cmd_buff, bytes_to_write);
	if (bytes_to_write != bytes_written)
		log_write(LOG_ERR, "sending CMD failed (only %d/%d bytes sent)", bytes_written, bytes_to_write);


	return NO_ERROR;
}
