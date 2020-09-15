#ifndef FIFOS_H_
#define FIFOS_H_

#include <stdint.h>

#define DEST_GATEWAY_APP	0
#define DEST_MAG			1

typedef struct
{
	uint8_t cmddest;
	uint8_t	cmd;
	uint32_t dat;
} cmd_t;

int fifo_create_and_open(void);
int fifo_rdcmd(cmd_t *cmd_buff, int *p_cmd_buff_widx);
int fifo_errorhandler(int rdwr_ret, int *fd);

#endif /* FIFOS_H_ */
