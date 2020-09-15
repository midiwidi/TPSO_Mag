#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "fifos.h"
#include "config.h"
#include "helpers.h"
#include "globals.h"
#include "log.h"

#define LF 0x0A
#define BUFFERSIZE 64


int fifo_create_and_open(void)
{
	if((mkfifo(CMD_FIFO, 0644)) == -1)
	{
		if (errno!=EEXIST)	// File exists
		{
			log_write(LOG_ERR,"can't create CMD fifo %s. errno %d (%s)", CMD_FIFO, errno,strerror(errno));
			return ERROR;
		}
	}

	g_fd_fifocmd = open(CMD_FIFO, O_RDONLY | O_NONBLOCK);
	if (g_fd_fifocmd < 0)
	{
		log_write(LOG_ERR, "can't open CMD fifo %s. errno %d (%s)", CMD_FIFO, errno,strerror(errno));
		return ERROR;
	}

	return NO_ERROR;
}

int fifo_rdcmd(cmd_t *cmd_buff, int *p_cmd_buff_widx)
{
	int ret = NO_DATA;
	char char_in;  					// Input buffer
	int nbytes;       				// Number of bytes read
	static char str[BUFFERSIZE] = "";
	static int cnt = 0;
	int n, dummy;
	int cmddest, cmd;
	uint32_t dat;

	while ((nbytes = read(g_fd_fifocmd, &char_in, sizeof(char))) > 0)
	{
		if (char_in == LF) //commands are delemited by line feed
		{
			str[cnt] = '\0'; //add string end
			n = sscanf(str, "%d %d %d %d", &cmddest, &cmd, &dat, &dummy);
			if (n == 3)	//only if exactly 3 values are read, it could be a valid command
			{
				(cmd_buff + (*p_cmd_buff_widx))->cmddest = cmddest;
				(cmd_buff + (*p_cmd_buff_widx))->cmd = cmd;
				(cmd_buff + (*p_cmd_buff_widx))->dat = dat;

				*p_cmd_buff_widx = (*p_cmd_buff_widx + 1) % CMD_BUF_DEPTH;
				ret = NO_ERROR;
			}
			cnt = 0;
			break;
		}
		else
		{
			str[cnt] = char_in;
			cnt = (cnt + 1) % BUFFERSIZE;
		}
	}
	if (nbytes < 0)
		ret = ERROR;
	return ret;
}

//********************************************************************************
int fifo_errorhandler(int rdwr_ret, int *fd)
{
	int ret;

	ret = rdwr_ret;
	if (rdwr_ret < 0)
	{
		if (errno == EAGAIN) /* Try again */
		{
			ret = NO_ERROR;
		}
		if (errno == EPIPE) /* Broken pipe */
		{
			close(*fd);
			*fd = -1;
			ret = ERROR;
		}
	}
	return ret;
}
