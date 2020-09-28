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
#include "toml.h"

const char* c_rate_str[] = {"100", "50", "10", "1"};

int main(int argc, char **argv)
{
	int opt;
	int option_index;
	int ret = NO_ERROR;

	long long_tmp;
	double dbl_tmp;
	char c_tmp;
	char str16_tmp[16];
	size_t sizet_tmp;

	uint16_t bytes_read;
	uint16_t stream_idx;
	time_t t_seconds;
	double t_subseconds;
	struct tm *ptm;
	useconds_t loop_sleep_time = DEFAULT_LOOP_SLEEP_TIME;
	uint8_t settime_req = 0;

	uint8_t byte_stream[STREAM_BUFF_LEN];
	uint8_t packet_len;
	uint8_t packet[PACKET_BUFF_LEN];

	cmd_t cmd_buff[CMD_BUF_DEPTH];
	int cmd_buff_widx = 0;
	int cmd_buff_ridx = 0;

	time_t t1 = 0, t2 = 0, t_set = 0, t_cmd_set = 0, t_check = 0, t_min = 0, t_max = 0;
	uint8_t setting_time_no_output = FALSE;

	uint8_t dbg_data_src = DBG_DATA_OFF;
	FILE *fid, *fid_minmax;

	static struct option long_options[] =
	{
		{ "avg_samp_bfield",	required_argument,	0, 'a' },
		{ "avg_samp_hk",		required_argument,	0, 'A' },
		{ "config",				required_argument, 	0, 'c' },
		{ "dbg_data_src",		required_argument,	0, 'D' },
		{ "help", 				no_argument,		0, 'h' },
		{ "loglevel", 			required_argument,	0, 'l' },
		{ "ser_port_dev",		required_argument,	0, 's' },
		{ "version", 			no_argument,		0, 'v' },
		{ 0, 0, 0, 0 }
	};
	char *optstring="a:A:c:D:hl:s:v";

	log_start(g_config.loglevel);
	signal_init();

	// generate version number from 3 octets git hash
	// a negative value means "dirty"
	strcpy(str16_tmp, VERSION);
	sizet_tmp = strlen(str16_tmp) - 1;
	if (sizet_tmp < 0)
		sizet_tmp = 0;
	c_tmp = str16_tmp[sizet_tmp];
	str16_tmp[sizet_tmp] = '\0';
	long_tmp = strtol(str16_tmp, 0, 16);
	if (c_tmp == 'y')
		long_tmp = -long_tmp;
	g_hk_data.app_version = long_tmp;

	while (1)
	{
		opt = getopt_long(argc, argv, optstring, long_options, &option_index);

		/* Detect the end of the options. */
		if (opt == -1)
			break;
		switch (opt)
		{
			case '?':
				/* unknown option. getopt_long already printed an error message. */
				print_cmd_usage();
				ret = ERROR;
				clean_exit(ret);
				break;
			case 'c':
				g_conf_file = malloc(strlen(optarg)+1);
				if(!g_conf_file)
				{
					log_write(LOG_ERR, "malloc failed getting memory for the config file name");
					ret = ERROR;
					clean_exit(ret);
				}
				strcpy(g_conf_file, optarg);
				break;
			case 'v':
				setlogmask(LOG_UPTO (LOG_NOTICE));
				g_config.loglevel = LOG_NOTICE;
				if (g_hk_data.app_version < 0)
					log_write(LOG_NOTICE, "version (Git hash): %07x-dirty", -g_hk_data.app_version);
				else
					log_write(LOG_NOTICE, "version (Git hash): %07x", g_hk_data.app_version);
				clean_exit(QUITE);
				break;
			default:
				break;
		}
	}

	ret = read_config(g_conf_file, &g_config);
	if (ret)
		log_write(LOG_WARNING, "reading config file: failed - using default config");
	else
		log_write(LOG_NOTICE, "reading config file: OK");

	// setting the global variable optind to 1 resets getopt_long() so that we can parse the options again
	optind = 1;

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
				if (limit_range_lng(&long_tmp, 0, 8640000))
					log_write(LOG_WARNING, "number of averaged b-field samples is out of range, set to %ld", long_tmp);
				g_config.avg_N_bfield = long_tmp;
				break;
			case 'A':
				long_tmp = strtol(optarg, 0, 0);
				if (limit_range_lng(&long_tmp, 0, 86400))
					log_write(LOG_WARNING, "number of averaged HK samples is out of range, set to %ld", long_tmp);
				g_config.avg_N_hk = long_tmp;
				break;
			case 'D':
				long_tmp = strtol(optarg, 0, 0);
				if (limit_range_lng(&long_tmp, 0, DBG_ALL))
					log_write(LOG_WARNING, "debug data source out of range, set to %ld", long_tmp);
				dbg_data_src = long_tmp;
				break;
			case 'h':
				print_cmd_usage();
				clean_exit(NO_ERROR);
				break;
			case 'l':
				long_tmp = strtol(optarg, 0, 0);
				if (limit_range_lng(&long_tmp, LOG_EMERG, LOG_DEBUG))
					log_write(LOG_WARNING, "loglevel out of range, limited to %ld", long_tmp);
				g_config.loglevel = long_tmp;
				setlogmask(LOG_UPTO(g_config.loglevel));
				break;
			case 's':
				strncpy(g_config.serial_port_device, optarg, strlen(g_config.serial_port_device));
				break;
			default:
				break;
		}
	}

	setlogmask(LOG_UPTO (g_config.loglevel));

	send_cmd(cmd.BAUD_RATE_DIV, (MAG_BAUDDIV_615384-1)*8 );		//Auflösung ist 1/8 = 0.125
	usleep(100000);
	send_cmd(cmd.BAUD_RATE_DIV, (MAG_BAUDDIV_615384-1)*8 );		//Auflösung ist 1/8 = 0.125
	usleep(100000);

	if(g_ser_port>=0) close(g_ser_port);
	g_ser_port = open_serial(g_config.serial_port_device, 615384);
	if (g_ser_port < 0)
	{
		log_write(LOG_ERR, "can't open device %s  at a baud rate of 615384", g_config.serial_port_device);
		clean_exit(ERROR);
	}

	// Set magnetometer data rate to 10 Hz
	send_cmd(cmd.RATE, 2);
	usleep(100000);
	send_cmd(cmd.RATE, 2);

	memset(&g_bfield_data, 0, sizeof(g_bfield_data));

	settime_req = TRUE;

	find_minmax(CLEAR, 0, &g_bfield_data.bx_min, &g_bfield_data.bx_max);
	find_minmax(CLEAR, 0, &g_bfield_data.by_min, &g_bfield_data.by_max);
	find_minmax(CLEAR, 0, &g_bfield_data.bz_min, &g_bfield_data.bz_max);

	find_minmax(CLEAR, 0, &g_hk_data.temp_e_min, &g_hk_data.temp_e_max);
	find_minmax(CLEAR, 0, &g_hk_data.temp_s_min, &g_hk_data.temp_s_max);
	find_minmax(CLEAR, 0, &g_hk_data.tilt_x_min, &g_hk_data.tilt_x_max);
	find_minmax(CLEAR, 0, &g_hk_data.tilt_y_min, &g_hk_data.tilt_y_max);
	find_minmax(CLEAR, 0, &g_hk_data.V5p_min, &g_hk_data.V5p_max);
	find_minmax(CLEAR, 0, &g_hk_data.V5n_min, &g_hk_data.V5n_max);
	find_minmax(CLEAR, 0, &g_hk_data.V33_min, &g_hk_data.V33_max);
	find_minmax(CLEAR, 0, &g_hk_data.V15_min, &g_hk_data.V15_max);

	// empty receive buffer (using 2 bytes, because for some reason I always get at least one)
	while(read(g_ser_port, &c_tmp, 2) >= 2);

	while(1)
	{
		bytes_read = read(g_ser_port, byte_stream, STREAM_BUFF_LEN);
		for(stream_idx=0; stream_idx < bytes_read; stream_idx++)
		{
			if (dbg_data_src & DBG_BYTE_STREAM) print_stream(byte_stream[stream_idx], NO_FLUSH);

			packet_len = slip_decode(byte_stream[stream_idx], packet);

			g_bfield_data.updated = 0;
			g_hk_data.updated = 0;

			if (g_startup) //prevent partial packets at startup
				g_startup = FALSE;
			else
			{
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
			}
			if (g_bfield_data.updated)
			{
				if (setting_time_no_output && !settime_req)
					setting_time_no_output--;

				if (setting_time_no_output == 0 && !settime_req)
				{

					log_write(LOG_INFO, "t=%.6f, BX=%10.3f, BY=%10.3f, BZ=%10.3f, status=0x%02x", g_bfield_data.t, g_bfield_data.bx, g_bfield_data.by, g_bfield_data.bz, g_bfield_data.status.raw);

					fid = fopen(g_config.mag_output_file,"a");
					if (!fid)
					{
						log_write(LOG_ERR, "%s open error", g_config.mag_output_file);
						clean_exit(ERROR);
					}
					fid_minmax = fopen(g_config.mag_minmax_file,"a");
					if (!fid_minmax)
					{
						log_write(LOG_ERR, "%s open error", g_config.mag_minmax_file);
						clean_exit(ERROR);
					}

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
					fprintf(fid_minmax, "%04d-%02d-%02d "
									"%02d:%02d:%02d.%.0f\t"
									"%f\t%f\t%f\t"
									"%f\t%f\t%f\n",
									ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
									ptm->tm_hour, ptm->tm_min, ptm->tm_sec, t_subseconds,
									g_bfield_data.bx_min, g_bfield_data.by_min, g_bfield_data.bz_min,
									g_bfield_data.bx_max, g_bfield_data.by_max, g_bfield_data.bz_max);
					fclose(fid);
					fclose(fid_minmax);
				}
				find_minmax(CLEAR, 0, &g_bfield_data.bx_min, &g_bfield_data.bx_max);
				find_minmax(CLEAR, 0, &g_bfield_data.by_min, &g_bfield_data.by_max);
				find_minmax(CLEAR, 0, &g_bfield_data.bz_min, &g_bfield_data.bz_max);
			}

			if (g_hk_data.updated)
			{
				if (setting_time_no_output == 0 && !settime_req)
				{
					log_write(LOG_INFO, "t=%.6f, t_drift=%.6f, FW=%d, status=0x%04X, TE=%7.3f, TS=%7.3f, TiltX=%7.3f, TiltY=%7.3f, V5p=%5.3f, V5n=%5.3f, V3.3=%5.3f, V1.5=%5.3f, RdPtr=%d, WrPtr=%d, SoftVers=%06x%s, CfgVers=%d.%d",
										g_hk_data.t, g_hk_data.t_drift, g_hk_data.firmware_version, g_hk_data.status.raw,
										g_hk_data.temp_e, g_hk_data.temp_s, g_hk_data.tilt_x, g_hk_data.tilt_y,
										g_hk_data.V5p, g_hk_data.V5n, g_hk_data.V33, g_hk_data.V15,
										g_hk_data.rd_ptr, g_hk_data.wr_ptr,
										g_hk_data.app_version<0?-g_hk_data.app_version:g_hk_data.app_version, g_hk_data.app_version<0?"-dirty":"",
										g_hk_data.version_major, g_hk_data.version_minor);

					fid = fopen(g_config.HK_output_file,"a");
					if (!fid)
					{
						log_write(LOG_ERR, "%s open error", g_config.HK_output_file);
						clean_exit(ERROR);
					}
					fid_minmax = fopen(g_config.HK_minmax_file,"a");
					if (!fid_minmax)
					{
						log_write(LOG_ERR, "%s open error", g_config.HK_minmax_file);
						clean_exit(ERROR);
					}

					t_subseconds = modf(g_hk_data.t, &dbl_tmp) * 1000000;
					t_seconds = (time_t)dbl_tmp;

					ptm = gmtime (&t_seconds);
					fprintf(fid, 	"%04d-%02d-%02d "
									"%02d:%02d:%02d.%.0f\t"
									"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s\t%d\t"
									"%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t"
									"%d\t%d\t%d\t%d.%03d\t%d.%d\t"
									"%f\n",
									ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
									ptm->tm_hour, ptm->tm_min, ptm->tm_sec, t_subseconds,
									g_hk_data.status.bits.start,
									g_hk_data.status.bits.excit_off,
									g_hk_data.status.bits.recording,
									g_hk_data.status.bits.flash_recording_disabled,
									g_hk_data.status.bits.ol,
									g_hk_data.status.bits.peo_s1,
									g_hk_data.status.bits.sync_off,
									g_hk_data.status.bits.sync_src,
									g_hk_data.status.bits.crc_ok,
									g_hk_data.status.bits.boot_main,
									g_hk_data.status.bits.eeprom_page,
									c_rate_str[g_hk_data.status.bits.rate],
									g_hk_data.status.bits.debug,
									g_hk_data.temp_e,
									g_hk_data.temp_s,
									g_hk_data.tilt_x,
									g_hk_data.tilt_y,
									g_hk_data.V5p,
									g_hk_data.V5n,
									g_hk_data.V33,
									g_hk_data.V15,
									g_hk_data.rd_ptr,
									g_hk_data.wr_ptr,
									g_hk_data.app_version,
									g_hk_data.version_major,
									g_hk_data.version_minor,
									g_hk_data.firmware_version >> 4,
									g_hk_data.firmware_version & 0xF,
									g_hk_data.t_drift);
					fprintf(fid_minmax, "%04d-%02d-%02d "
									"%02d:%02d:%02d.%.0f\t"
									"%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t"
									"%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n",
									ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
									ptm->tm_hour, ptm->tm_min, ptm->tm_sec, t_subseconds,
									g_hk_data.temp_e_min,
									g_hk_data.temp_s_min,
									g_hk_data.tilt_x_min,
									g_hk_data.tilt_y_min,
									g_hk_data.V5p_min,
									g_hk_data.V5n_min,
									g_hk_data.V33_min,
									g_hk_data.V15_min,
									g_hk_data.temp_e_max,
									g_hk_data.temp_s_max,
									g_hk_data.tilt_x_max,
									g_hk_data.tilt_y_max,
									g_hk_data.V5p_max,
									g_hk_data.V5n_max,
									g_hk_data.V33_max,
									g_hk_data.V15_max);
					fclose(fid);
					fclose(fid_minmax);
				}

				find_minmax(CLEAR, 0, &g_hk_data.temp_e_min, &g_hk_data.temp_e_max);
				find_minmax(CLEAR, 0, &g_hk_data.temp_s_min, &g_hk_data.temp_s_max);
				find_minmax(CLEAR, 0, &g_hk_data.tilt_x_min, &g_hk_data.tilt_x_max);
				find_minmax(CLEAR, 0, &g_hk_data.tilt_y_min, &g_hk_data.tilt_y_max);
				find_minmax(CLEAR, 0, &g_hk_data.V5p_min, &g_hk_data.V5p_max);
				find_minmax(CLEAR, 0, &g_hk_data.V5n_min, &g_hk_data.V5n_max);
				find_minmax(CLEAR, 0, &g_hk_data.V33_min, &g_hk_data.V33_max);
				find_minmax(CLEAR, 0, &g_hk_data.V15_min, &g_hk_data.V15_max);
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
						if (limit_range_lng(&long_tmp, 0, 8640000))
							log_write(LOG_WARNING, "number of averaged b-field samples is out of range, set to %ld", long_tmp);
						g_config.avg_N_bfield = long_tmp;
						break;
					case APP_CMD_AVG_SAMP_HK:
						if (limit_range_lng(&long_tmp, 0, 86400))
							log_write(LOG_WARNING, "number of averaged HK samples is out of range, set to %ld", long_tmp);
						g_config.avg_N_hk = long_tmp;
						break;
					case APP_CMD_LOGLEVEL:
						if (limit_range_lng(&long_tmp, LOG_EMERG, LOG_DEBUG))
							log_write(LOG_WARNING, "loglevel out of range, limited to %ld", long_tmp);
						g_config.loglevel = long_tmp;
						setlogmask(LOG_UPTO(g_config.loglevel));
						break;
					case APP_CMD_READ_CFG:
						ret = read_config(g_conf_file, &g_config);
						if (ret)
							log_write(LOG_WARNING, "reading config file: failed - using default config");
						else
							log_write(LOG_NOTICE, "reading config file: OK");
						break;
					case APP_SET_TIME:
						settime_req = TRUE;
						t_cmd_set = long_tmp;
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

				if (cmd_buff[cmd_buff_ridx].cmd == cmd.MAG_CFG_REG && (cmd_buff[cmd_buff_ridx].dat & RESET_BIT) )
				{
					g_startup = TRUE;
					sleep(3); // Magnetometer needs 2s to boot -> wait 3s before issuing SW commands
					if(g_ser_port>=0) close(g_ser_port);
					g_ser_port = open_serial(g_config.serial_port_device, 115200);
					if (g_ser_port < 0)
					{
						log_write(LOG_ERR, "can't open device %s  at a baud rate of 115200", g_config.serial_port_device);
						clean_exit(ERROR);
					}
					send_cmd(cmd.BAUD_RATE_DIV, (MAG_BAUDDIV_615384-1)*8 );		//Auflösung ist 1/8 = 0.125

					if(g_ser_port>=0) close(g_ser_port);
					g_ser_port = open_serial(g_config.serial_port_device, 615384);
					if (g_ser_port < 0)
					{
						log_write(LOG_ERR, "can't open device %s  at a baud rate of 615384", g_config.serial_port_device);
						clean_exit(ERROR);
					}
				}
			}

			cmd_buff_ridx = (cmd_buff_ridx+1) % CMD_BUF_DEPTH;
		}
		/* CMD Processing <end> *********************************************************************************************/

		/* Set Time if requested ********************************************************************************************/
		if (settime_req)
		{
			if (!t1)
			{
				t1 = time(NULL);
				loop_sleep_time = 0;
				//log_write(LOG_NOTICE, "t1=%ld", t1);
			}
			// Wait for the second to change so that there is enough time until the next 1PPS
			if ( (t2 = time(NULL)) != t1)
			{
				if (!t_check) // was time already set and we are waiting to check if is successful?
				{
					t_check = t2 + 1; // check in TIMESYNC_CHECK_SAMP samples if time was set correctly
					t2++; // increment the time by 1 as it is for the next 1PPS
					if (t_cmd_set) // if specific time was supplied, use the current time
						t_set = t_cmd_set;
					else
						t_set = t2;

					//log_write(LOG_NOTICE, "t2=%ld, t_set=%ld, t_check=%ld", t2, t_set, t_check);
					setting_time_no_output = NO_OUTPUT_FOR_N_SAMP_AFTER_TIMESET;

					send_cmd(cmd.TIMESTAMP_MSW, t_set >> 16);
					usleep(1000);
					send_cmd(cmd.TIMESTAMP_LSW, t_set & 0xFFFF);

					loop_sleep_time = DEFAULT_LOOP_SLEEP_TIME;
				}
				else
				{
					if (time(NULL) >= t_check)
					{
						t_max = t_set + 1;
						t_min = t_set - 1;
						//log_write(LOG_NOTICE, "check: t_set_min=%ld, t_set_max=%ld, mag.t=%f", t_set, t_set + TIMESYNC_CHECK_SAMP * g_config.avg_N_bfield / DEFAULT_MAG_RATE, g_bfield_data.t);
						if ( (g_mag_time >= t_min) && (g_mag_time <= t_max) )
						{
							settime_req = FALSE;
							log_write(LOG_NOTICE,"Time successfully set (min=%ld, max=%ld, mag=%.1f)", t_set, t_max, g_mag_time);
						}
						else
						{
							settime_req = TRUE;
							log_write(LOG_WARNING,"Setting time unsuccessful (min=%ld, max=%ld, mag=%.1f) ... retrying",t_set, t_max, g_mag_time);
						}
						t1 = 0;
						t2 = 0;
						t_set = 0;
						t_check = 0;
					}
				}
			}
		}
		/* Set Time if requested <end> **************************************************************************************/

		usleep(loop_sleep_time);
	}
}
