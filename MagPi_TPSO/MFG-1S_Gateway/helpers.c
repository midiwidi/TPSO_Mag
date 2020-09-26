#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <float.h>
#include <time.h>
#include <math.h>

#include "helpers.h"
#include "log.h"
#include "globals.h"
#include "config.h"
#include "serial.h"

int limit_range_lng(long *value, long llimit, long ulimit)
{
	if (*value < llimit)
	{
		*value = llimit;
		return ERROR;
	}
	if (*value > ulimit)
	{
		*value = ulimit;
		return ERROR;
	}
	return NO_ERROR;
}

int limit_range_dbl(double *value, double llimit, double ulimit)
{
	if (*value < llimit)
	{
		*value = llimit;
		return ERROR;
	}
	if (*value > ulimit)
	{
		*value = ulimit;
		return ERROR;
	}
	return NO_ERROR;
}

void find_minmax(int operation, double current_value, double *minimum_value, double *maximum_value)
{
	if (operation == CLEAR)
	{
		*minimum_value = DBL_MAX;
		*maximum_value = -DBL_MAX;
	}
	else
	{
		if (current_value < *minimum_value)
			*minimum_value = current_value;
		if (current_value > *maximum_value)
				*maximum_value = current_value;
	}
}

void clean_exit(int ret)
{
	time_t t_seconds;
	double t_subseconds;
	struct tm *ptm;
	double dbl_tmp;
	FILE *fid;

	/************************************************************************************************/
	/* Output one vector of NULLs at the end of each file to be pushed into the database. This will */
	/* make sure that a gap will be visible in the Grafana plots                                    */
	/************************************************************************************************/
	t_subseconds = modf(g_bfield_data.t, &dbl_tmp) * 1000000 + NULLS_DELTA_TSUBSEC;
	t_seconds = (time_t)dbl_tmp;

	ptm = gmtime (&t_seconds);
	if (g_config.mag_output_file)
	{
		fid = fopen(g_config.mag_output_file,"a");
		if (!fid)
		{
			log_write(LOG_ERR, "%s open error", g_config.mag_output_file);
			free(g_config.mag_output_file);
		}
		fprintf(fid,"%04d-%02d-%02d "
					"%02d:%02d:%02d.%.0f\t"
					"NULL\tNULL\tNULL\t"
					"NULL\tNULL\tNULL\t"
					"NULL\n",
					ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
					ptm->tm_hour, ptm->tm_min, ptm->tm_sec, t_subseconds);
		fclose(fid);
		free(g_config.mag_output_file);
	}

	if (g_config.mag_minmax_file)
	{
		fid = fopen(g_config.mag_minmax_file,"a");
		if (!fid)
		{
			log_write(LOG_ERR, "%s open error", g_config.mag_minmax_file);
			free(g_config.mag_minmax_file);
		}
		fprintf(fid,"%04d-%02d-%02d "
					"%02d:%02d:%02d.%.0f\t"
					"NULL\tNULL\tNULL\t"
					"NULL\tNULL\tNULL\n",
					ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
					ptm->tm_hour, ptm->tm_min, ptm->tm_sec, t_subseconds);
		fclose(fid);
		free(g_config.mag_minmax_file);
	}

	t_subseconds = modf(g_hk_data.t, &dbl_tmp) * 1000000 + NULLS_DELTA_TSUBSEC;
	t_seconds = (time_t)dbl_tmp;
	ptm = gmtime (&t_seconds);

	if (g_config.HK_output_file)
	{
		fid = fopen(g_config.HK_output_file,"a");
		if (!fid)
		{
			log_write(LOG_ERR, "%s open error", g_config.HK_output_file);
			free(g_config.HK_output_file);
		}
		fprintf(fid,"%04d-%02d-%02d "
					"%02d:%02d:%02d.%.0f\t"
					"NULL\tNULL\tNULL\tNULL\tNULL\tNULL\tNULL\tNULL\tNULL\tNULL\tNULL\tNULL\tNULL\t"
					"NULL\tNULL\tNULL\tNULL\tNULL\tNULL\tNULL\tNULL\t"
					"NULL\tNULL\tNULL\tNULL\tNULL\tNULL\n",
					ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
					ptm->tm_hour, ptm->tm_min, ptm->tm_sec, t_subseconds);
		fclose(fid);
		free(g_config.HK_output_file);
	}

	if (g_config.HK_minmax_file)
	{
		fid = fopen(g_config.HK_minmax_file,"a");
		if (!fid)
		{
			log_write(LOG_ERR, "%s open error", g_config.HK_minmax_file);
			free(g_config.HK_minmax_file);
		}
		fprintf(fid,"%04d-%02d-%02d "
					"%02d:%02d:%02d.%.0f\t"
					"NULL\tNULL\tNULL\tNULL\tNULL\tNULL\tNULL\tNULL\t"
					"NULL\tNULL\tNULL\tNULL\tNULL\tNULL\tNULL\tNULL\n",
					ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
					ptm->tm_hour, ptm->tm_min, ptm->tm_sec, t_subseconds);
		fclose(fid);
		free(g_config.HK_minmax_file);
	}
	/* Output NULL vectors <end> *********************************************************************/

	if (ret != QUITE)
	{
		if (ret)
			log_write(LOG_ERR, "MFG-1S_Gateway exited off-nominally", ret);
		else
			log_write(LOG_NOTICE, "MFG-1S_Gateway exited nominally");
	}
	closelog();

	if (g_fd_fifocmd >= 0)
		close(g_fd_fifocmd);
	unlink(CMD_FIFO);

	if (g_ser_port >= 0)
		close(g_ser_port);
	if (g_config.serial_port_device)
		free(g_config.serial_port_device);
	if (g_conf_file)
		free(g_conf_file);

	exit(ret);
}

int32_t extend_sign_bit(int32_t value, uint8_t width)
{
	uint32_t lTmp=-1;

	if ( value & (0x01<<(width - 1)) )
	{

		lTmp=lTmp<<(width);
		return (value | lTmp);

	}
	else
		return value;
}

void print_stream(uint8_t byte, uint8_t flush)
{
	static uint8_t val_on_line = 0;
	static char str_line[HEX_BYTES_PER_ROW * 5 + 1] = "";
	static uint8_t idx = 0;

	if (!flush)
	{
		snprintf(&str_line[idx], sizeof(str_line)-idx, "0x%02X ", byte);
		idx += 5;
		val_on_line++;
	}
	if (val_on_line == HEX_BYTES_PER_ROW || flush)
	{
		if (str_line[0])
			log_write(LOG_DEBUG, "%s", str_line);
		str_line[0] = '\0';
		val_on_line = 0;
		idx = 0;
	}
}

void print_packet(uint8_t* packet, uint8_t packet_len)
{
	uint8_t line_idx = 0;
	char str_line[PACKET_BUFF_LEN * 5 + 1] = "";
	uint8_t packet_idx = 0;

	for (packet_idx=0; packet_idx<packet_len; packet_idx++)
	{
		snprintf(&str_line[line_idx], sizeof(str_line)-line_idx, "0x%02X ", packet[packet_idx]);
		line_idx += 5;
	}
	log_write(LOG_DEBUG, "%s", str_line);
}

void print_cmd_usage(void)
{
	fprintf(stdout,
			"usage: MFG-1S_Gateway [OPTIONS]\n"
			"\n"
			"Receives data from a digital fluxgate magnetometer (model MFG-1S) manufactured by \"Magson GmbH\",\n"
			"decodes and converts the data and outputs the data into a files (one for B field data, one for HK).\n"
			"Note, all integer options can be provided in decimal, hexadecimal (prefix: 0x) or octal (prefix: 0) form.\n"
			"\n"
			"OPTIONS:\n"
			"-a, --avg_samp_bfield=N   Sets the number of averaged magnetic field samples to N.\n"
			"                          If the output rate of the magnetometer is for example 100 Hz\n"
			"                          and N is 6000, then the cadence of magnetic field samples published\n"
			"                          is 1 minute. N=0 disables the magnetic field output.\n"
			"                          Data type: Integer, Range: 0...8640000\n"
			"\n"
			"-A, --avg_samp_hk=N       Sets the number of averaged housekeeping samples to N.\n"
			"                          With N=60, HK messages will be published with a rate of 1 minute as\n"
			"                          the magnetometer HK output rate is 1Hz. N=0 disables the HK output.\n"
			"                          Data type: Integer, Range: 0...86400\n"
			"\n"
			"-D, --dbg_data_src        Sets the sources of raw data output for debugging.\n"
			"                          Each source is represented by 1 bit of the given value.\n"
			"                          bit0: byte stream data from magnetometer\n"
			"                          bit1: packet data (SLIP-decoded, FEND removed)\n"
			"                          Data type: Integer, Range: 0...3\n"
			"\n"
			"-h, --help                Prints this help information\n"
			"\n"
			"-l, --loglevel=SEVERITY   Sets the log level to SEVERITY. All messages with a level\n"
			"                          equal or less than SEVERITY will be put into syslog.\n"
			"                          SEVERITY must be one of the following integers\n"
			"                          0: emerg          4: warning (default)\n"
			"                          1: alert          5: notice\n"
			"                          2: crit           6: info\n"
			"                          3: error          7: debug\n"
			"                          Data type: Integer, Range: 0...7\n"
			"\n"
			"-v, --version             Prints version information\n"
			"\n"
			"-s, --ser_port_device     Sets the serial port device used to communicate with the\n"
			"                          magnetometer (default: /dev/ttyUSB0)\n"
	);
}
