#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "serial.h"
#include "globals.h"
#include "config.h"
#include "log.h"
#include "signal.h"
#include "helpers.h"
#include "packet.h"
#include "cmd.h"
#include "fifos.h"


int main(int argc, char **argv)
{
	int opt;
	int option_index;
	int ret = NO_ERROR;
	char serial_port_device[64] = "/dev/ttyUSB0";

	long long_tmp;
	double dbl_tmp;

	uint16_t bytes_read;
	uint16_t stream_idx;
	time_t t_seconds;
	double t_subseconds;
	struct tm * ptm;

	uint8_t byte_stream[STREAM_BUFF_LEN];
	uint8_t packet_len;
	uint8_t packet[PACKET_BUFF_LEN];

	cmd_t cmd_buff[CMD_BUF_DEPTH];
	int cmd_buff_widx = 0;
	int cmd_buff_ridx = 0;

	uint8_t dbg_data_src = DBG_DATA_OFF;
	FILE *fid;

	static struct option long_options[] =
	{
		{ "avg_samp_bfield",	required_argument,	0, 'a' },
		{ "avg_samp_hk",		required_argument,	0, 'A' },
		{ "dbg_data_src",		required_argument,	0, 'D' },
		{ "help", 				no_argument,		0, 'h' },
		{ "loglevel", 			required_argument,	0, 'l' },
		{ "ser_port_dev",		required_argument,	0, 's' },
		{ "version", 			no_argument,		0, 'v' },
		{ 0, 0, 0, 0 }
	};
	char *optstring="a:A:D:hl:s:v";

	log_start(g_config.loglevel);
	signal_init();

	while (1)
	{
		opt = getopt_long(argc, argv, optstring, long_options, &option_index);

		/* Detect the end of the options. */
		if(opt == -1)
			break;

		switch (opt)
		{
			case '?':
				/* unknown option. getopt_long already printed an error message. */
				print_cmd_usage();
				clean_exit(ERROR);
				break;
			case 'a':
				long_tmp = strtol(optarg, 0, 0);
				if (limit_range(&long_tmp, 0, 8640000))
					log_write(LOG_WARNING, "number of averaged b-field samples is out of range, set to %ld", long_tmp);
				g_config.avg_N_bfield = long_tmp;
				break;
			case 'A':
				long_tmp = strtol(optarg, 0, 0);
				if (limit_range(&long_tmp, 0, 86400))
					log_write(LOG_WARNING, "number of averaged HK samples is out of range, set to %ld", long_tmp);
				g_config.avg_N_hk = long_tmp;
				break;
			case 'D':
				long_tmp = strtol(optarg, 0, 0);
				if (limit_range(&long_tmp, 0, DBG_ALL))
					log_write(LOG_WARNING, "debug data source out of range, set to %ld", long_tmp);
				dbg_data_src = long_tmp;
				break;
			case 'h':
				print_cmd_usage();
				clean_exit(NO_ERROR);
				break;
			case 'l':
				long_tmp = strtol(optarg, 0, 0);
				if (limit_range(&long_tmp, LOG_EMERG, LOG_DEBUG))
					log_write(LOG_WARNING, "loglevel out of range, limited to %ld", long_tmp);
				g_config.loglevel = long_tmp;
				setlogmask(LOG_UPTO(g_config.loglevel));
				break;
			case 's':
				strncpy(serial_port_device, optarg, sizeof(serial_port_device));
				break;
			case 'v':
				fprintf(stdout, "version: %s\n", VERSION);
				clean_exit(QUITE);
				break;
			default:
				break;
		}
	}

	setlogmask(LOG_UPTO (g_config.loglevel));

	g_ser_port = open_serial(serial_port_device, 115200);
	if (g_ser_port < 0)
	{
		log_write(LOG_ERR, "can't open device %s at a baud rate of 115200", serial_port_device);
		clean_exit(ERROR);
	}

	send_cmd(cmd.BAUD_RATE_DIV, (MAG_BAUDDIV_615384-1)*8 );		//Auflösung ist 1/8 = 0.125

	if(g_ser_port>=0) close(g_ser_port);
	g_ser_port = open_serial(serial_port_device, 615384);
	if (g_ser_port < 0)
	{
		log_write(LOG_ERR, "can't open device %s  at a baud rate of 615384", serial_port_device);
		clean_exit(ERROR);
	}

	memset(&g_bfield_data, 0, sizeof(g_bfield_data));

	while(1)
	{

		bytes_read = read(g_ser_port, byte_stream, STREAM_BUFF_LEN);
		for(stream_idx=0; stream_idx < bytes_read; stream_idx++)
		{
			if (dbg_data_src & DBG_BYTE_STREAM) print_stream(byte_stream[stream_idx], NO_FLUSH);

			packet_len = slip_decode(byte_stream[stream_idx], packet);

			g_bfield_data.updated = 0;
			g_hk_data.updated = 0;

			if (packet_len > 0)
			{
				if (dbg_data_src & DBG_BYTE_STREAM) print_stream(0, FLUSH);
				if (dbg_data_src & DBG_PACKET) print_packet(packet, packet_len);

				ret = packet_decode(packet, packet_len);
				if (ret == ERROR)
				{
					g_packet_error_cnt++;
					log_write(LOG_WARNING, "packet errors: %d", g_packet_error_cnt);
				}

			}

			if (g_bfield_data.updated)
			{
				log_write(LOG_INFO, "t=%.6f, BX=%10.3f, BY=%10.3f, BZ=%10.3f, status=0x%02x", g_bfield_data.t, g_bfield_data.bx, g_bfield_data.by, g_bfield_data.bz, g_bfield_data.status.raw);

				fid = fopen("data/mag.dat","a");
				t_subseconds = modf(g_bfield_data.t, &dbl_tmp) * 1000000;
				t_seconds = (time_t)dbl_tmp;
				ptm = gmtime (&t_seconds);

				fprintf(fid, 	"%04d-%02d-%02d "
								"%02d:%02d:%02d.%.0f\t"
								"%f\t%f\t%f\t"
								"%d\t%d\t%d\t"
								"%d\n",
								ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
								ptm->tm_hour, ptm->tm_min, ptm->tm_sec, t_subseconds,
								g_bfield_data.bx, g_bfield_data.by, g_bfield_data.bz,
								g_bfield_data.status.bits.clip_x, g_bfield_data.status.bits.clip_y, g_bfield_data.status.bits.clip_z,
								g_bfield_data.status.bits.sync);
				fclose(fid);
			}

			if (g_hk_data.updated)
			{
				log_write(LOG_INFO, "t=%.6f, FW=%d, status=0x%04X, TE=%7.3f, TS=%7.3f, TiltX=%7.3f, TiltY=%7.3f, V5p=%5.3f, V5n=%5.3f, V3.3=%5.3f, V1.5=%5.3f, RdPtr=%d, WrPtr=%d",
									g_hk_data.t, g_hk_data.firmware_version, g_hk_data.status.raw,
									g_hk_data.temp_e, g_hk_data.temp_s, g_hk_data.tilt_x, g_hk_data.tilt_y,
									g_hk_data.V5p, g_hk_data.V5n, g_hk_data.V33, g_hk_data.V15,
									g_hk_data.rd_ptr, g_hk_data.wr_ptr);

				fid = fopen("data/hk.dat","a");
				fprintf(fid, "%f\t%d\t%d\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%d\t%d\n",
						g_hk_data.t, g_hk_data.firmware_version, g_hk_data.status.raw,
						g_hk_data.temp_e, g_hk_data.temp_s, g_hk_data.tilt_x, g_hk_data.tilt_y,
						g_hk_data.V5p, g_hk_data.V5n, g_hk_data.V33, g_hk_data.V15,
						g_hk_data.rd_ptr, g_hk_data.wr_ptr);
				fclose(fid);
			}

		}

		/* Read command from CMD FIFO **************************************************************************************/
		do
		{
			if (g_fd_fifocmd<0)
			{
				if (fifo_create_and_open()==ERROR)
					clean_exit(ERROR);
			}

			ret = fifo_rdcmd(cmd_buff, &cmd_buff_widx);
			ret = fifo_errorhandler(ret, &g_fd_fifocmd);
			if (ret==ERROR)
				log_write(LOG_ERR,"reading from CMD fifo failed. errno=%d (%s)",errno,strerror(errno));
		} while (ret==NO_ERROR);
		/* Read command from CMD FIFO <end>*********************************************************************************/

		/* CMD Processing **************************************************************************************************/
		while (cmd_buff_widx != cmd_buff_ridx)
		{
			log_write(LOG_NOTICE,"CMD received (CMD_Dest=%d, CMD_ID=%d, CMD_Dat=%lu)",
					cmd_buff[cmd_buff_ridx].cmddest,
					cmd_buff[cmd_buff_ridx].cmd,
					cmd_buff[cmd_buff_ridx].dat);

			if (cmd_buff[cmd_buff_ridx].cmddest == DEST_GATEWAY_APP)
			{
				// command for the gateway application
				long_tmp = cmd_buff[cmd_buff_ridx].dat;
				switch(cmd_buff[cmd_buff_ridx].cmd)
				{
					case APP_CMD_AVG_SAMP_BFIELD:
						if (limit_range(&long_tmp, 0, 8640000))
							log_write(LOG_WARNING, "number of averaged b-field samples is out of range, set to %ld", long_tmp);
						g_config.avg_N_bfield = long_tmp;
						break;
					case APP_CMD_AVG_SAMP_HK:
						if (limit_range(&long_tmp, 0, 86400))
							log_write(LOG_WARNING, "number of averaged HK samples is out of range, set to %ld", long_tmp);
						g_config.avg_N_hk = long_tmp;
						break;
					case APP_CMD_LOGLEVEL:
						if (limit_range(&long_tmp, LOG_EMERG, LOG_DEBUG))
							log_write(LOG_WARNING, "loglevel out of range, limited to %ld", long_tmp);
						g_config.loglevel = long_tmp;
						setlogmask(LOG_UPTO(g_config.loglevel));
						break;
					case APP_CMD_EXIT:
						clean_exit(NO_ERROR);
						break;
					default:
						log_write(LOG_WARNING, "warning: unknown gateway app command");
						break;
				}
			}
			else
			{
				// command for magnetometer
				send_cmd(cmd_buff[cmd_buff_ridx].cmd, cmd_buff[cmd_buff_ridx].dat);
			}

			cmd_buff_ridx = (cmd_buff_ridx+1) % CMD_BUF_DEPTH;
		}
		/* CMD Processing <end> *********************************************************************************************/

		usleep(1000);
	}
}
