#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <tomlc99/toml.h>

#include "log.h"
#include "helpers.h"
#include "toml.h"
#include "data_processing.h"
#include "globals.h"
#include "serial.h"

void toml_clean_exit(int ret, FILE *toml_fp, toml_table_t *toml_conf)
{
	if (toml_fp)
	{
		fclose(toml_fp);
		toml_fp = NULL;
	}

	if (toml_conf)
	{
		toml_free(toml_conf);
		toml_conf = NULL;
	}

	clean_exit(ret);
}

int toml_readkey_int(toml_table_t* toml_tab, const char *key, int64_t *toml_int64)
{
	const char* toml_raw;

	/* extract key value */
	if (!(toml_raw = toml_raw_in(toml_tab, key)))
	{
		log_write(LOG_ERR, "can't find configuration key %s", key);
		return ERROR;
	}
	/* convert to integer value */
	if (toml_rtoi(toml_raw, toml_int64))
	{
		log_write(LOG_ERR, "toml_rtoi error");
		return ERROR;
	}
	return NO_ERROR;
}

int toml_readkey_dbl(toml_table_t* toml_tab, const char *key, double *output)
{
	const char* toml_raw;

	/* extract key value */
	if (!(toml_raw = toml_raw_in(toml_tab, key)))
	{
		log_write(LOG_ERR, "can't find configuration key %s", key);
		return ERROR;
	}
	/* convert to double value */
	if (toml_rtod(toml_raw, output))
	{
		log_write(LOG_ERR, "toml_rtod error");
		return ERROR;
	}
	return NO_ERROR;
}

int toml_readkey_str(toml_table_t* toml_tab, const char *key, char **str)
{
	const char* toml_raw;

	/* extract key value */
	if (!(toml_raw = toml_raw_in(toml_tab, key)))
	{
		log_write(LOG_ERR, "can't find configuration key %s", key);
		return ERROR;
	}
	/* convert to string */
	if (toml_rtos(toml_raw, str))
	{
		log_write(LOG_ERR, "toml_rtos error");
		return ERROR;
	}
	return NO_ERROR;
}

int read_config(char* conf_file, struct config *cfg)
{
	FILE* toml_fp = NULL;
	char toml_errbuf[200];
	toml_table_t* toml_conf = NULL;
	toml_table_t* toml_tab = NULL;

	int64_t toml_int64;
	long long_tmp;
	double dbl_tmp;
	int ret = NO_ERROR;

	if (!(toml_fp = fopen(conf_file, "r")))
	{
		// open conf file specified via parameter failed
		if (!(toml_fp = fopen(DEFAULT_CONFIG_FILE, "r")))
		{
			// if no config file was specified and the default config file couldn't be found,
			// then initialize config with default values
			cfg->avg_N_bfield = 6000;
			cfg->avg_N_hk = 240;
			cfg->downsample_mode = AVERAGING;
			cfg->loglevel = LOG_NOTICE;
			//cfg->offset //done at struct declaration
			//cfg->orth = //done at struct declaration
			//cfg->scale = //done at struct declaration
			//cfg->telemetry_resolution = //done at struct declaration
			cfg->timestamp_position = TIMESTAMP_AT_CENTER;
			g_hk_data.version_major = 0;
			g_hk_data.version_minor = 0;
			g_hk_data.app_version = 0;
			asprintf(&cfg->serial_port_device, "/dev/ttyUSB0");
			asprintf(&cfg->mag_output_file, "data/mag.dat");
			asprintf(&cfg->mag_minmax_file, "data/mag.dat.minmax");
			asprintf(&cfg->HK_output_file, "data/hk.dat");
			asprintf(&cfg->HK_minmax_file, "data/hk.dat.minmax");

			if (g_conf_file)
					free(conf_file);
			return NO_ERROR;
		}
		else
			// open default conf file succeeded
			log_write(LOG_INFO, "using default config file %s", DEFAULT_CONFIG_FILE);
	}
	else
		// open conf file specified via parameter succeeded
		log_write(LOG_INFO, "using config file %s", conf_file);

	toml_conf = toml_parse_file(toml_fp, toml_errbuf, sizeof(toml_errbuf));

	fclose(toml_fp);
	toml_fp = NULL;

	if (!toml_conf)
	{
		log_write(LOG_ERR, "error parsing the config file (%s)", toml_errbuf);
		ret = ERROR;
		toml_clean_exit(ret, toml_fp, toml_conf);
	}

	if((ret=toml_readkey_int(toml_conf, "version_major", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	g_hk_data.version_major = toml_int64;
	if((ret=toml_readkey_int(toml_conf, "version_minor", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	g_hk_data.version_minor = toml_int64;

	// Reading the configuration from the *.toml file is divided into several blocks
	// This is done to make sure that the order in which keys are read is such that
	// keys which other keys depend on are read first before the key which depend on
	// other keys are read.

	/********************************************************************************/
	/* 1st Block: keys, which don't depend on other keys                            */
	/********************************************************************************/
	// locate the [Gateway] table
	if (!(toml_tab = toml_table_in(toml_conf, "Gateway")))
	{
		log_write(LOG_ERR, "can't find configuration table 'Gateway'");
		ret = ERROR;
		toml_clean_exit(ret, toml_fp, toml_conf);
	}

	if((ret=toml_readkey_int(toml_tab, "Bfield_Divider", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	long_tmp = toml_int64;
	if (limit_range_lng(&long_tmp, 0, 8640000))
		log_write(LOG_WARNING, "Magnetic field sample rate divider is out of range and was set to %ld", toml_int64);
	g_config.avg_N_bfield = toml_int64;

	if((ret=toml_readkey_int(toml_tab, "HK_Divider", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	long_tmp = toml_int64;
	if (limit_range_lng(&long_tmp, 0, 86400))
		log_write(LOG_WARNING, "HK sample rate divider is out of range and was set to %ld", toml_int64);
	g_config.avg_N_hk = toml_int64;

	if((ret=toml_readkey_int(toml_tab, "Downsampling_Mode", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	long_tmp = toml_int64;
	if (limit_range_lng(&long_tmp, 0, 1))
	{
		log_write(LOG_WARNING, "Downsampling mode can only be 0 or 1 and was set to 0", toml_int64);
		toml_int64 = AVERAGING;
	}
	g_config.downsample_mode = toml_int64;

	if((ret=toml_readkey_int(toml_tab, "Timestamp_Position", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	long_tmp = toml_int64;
	if (limit_range_lng(&long_tmp, TIMESTAMP_AT_FIRST_SAMPLE, TIMESTAMP_AT_LAST_SAMPLE))
	{
		log_write(LOG_WARNING, "Timestamp Position can only be 0, 1 or 2 and was set to 1", toml_int64);
		toml_int64 = TIMESTAMP_AT_CENTER;
	}
	g_config.timestamp_position = toml_int64;

	if((ret=toml_readkey_int(toml_tab, "Loglevel", &toml_int64)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	long_tmp = toml_int64;
	if (limit_range_lng(&long_tmp, 0, 7))
		log_write(LOG_WARNING, "Log level is out of range and was set to %ld", toml_int64);
	g_config.loglevel = toml_int64;
	setlogmask(LOG_UPTO(g_config.loglevel));

	if((ret=toml_readkey_dbl(toml_tab, "Telemetry_Res", &dbl_tmp)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	if (limit_range_dbl(&dbl_tmp, 0.0, 1.0))
		log_write(LOG_WARNING, "Telemetry resolution is out of range and was set to %f", dbl_tmp);
	g_config.telemetry_resolution = dbl_tmp;

	if (cfg->serial_port_device == NULL) // don't change the serial port during re-reading the config file
	{
		if((ret=toml_readkey_str(toml_tab, "Serial_Port", &cfg->serial_port_device)))
			toml_clean_exit(ret, toml_fp, toml_conf);
		if (g_ser_port >= 0)
			close(g_ser_port);
		g_ser_port = open_serial(g_config.serial_port_device, 115200);
		if (g_ser_port < 0)
		{
			log_write(LOG_ERR, "can't open device %s at a baud rate of 115200", g_config.serial_port_device);
			clean_exit(ERROR);
		}
	}

	if((ret=toml_readkey_str(toml_tab, "Mag_File", &cfg->mag_output_file)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	asprintf(&cfg->mag_minmax_file, "%s.minmax", cfg->mag_output_file);

	if((ret=toml_readkey_str(toml_tab, "HK_File", &cfg->HK_output_file)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	asprintf(&cfg->HK_minmax_file, "%s.minmax", cfg->HK_output_file);

	// locate the [Calibration] table
	if (!(toml_tab = toml_table_in(toml_conf, "Calibration")))
	{
		log_write(LOG_ERR, "can't find configuration table 'Calibration'");
		ret = ERROR;
		toml_clean_exit(ret, toml_fp, toml_conf);
	}

	if((ret=toml_readkey_dbl(toml_tab, "Cal_Scale_BX", &g_config.scale.x)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	if((ret=toml_readkey_dbl(toml_tab, "Cal_Scale_BY", &g_config.scale.y)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	if((ret=toml_readkey_dbl(toml_tab, "Cal_Scale_BZ", &g_config.scale.z)))
		toml_clean_exit(ret, toml_fp, toml_conf);

	if((ret=toml_readkey_dbl(toml_tab, "Cal_Offset_BX", &g_config.offset.x)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	if((ret=toml_readkey_dbl(toml_tab, "Cal_Offset_BY", &g_config.offset.y)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	if((ret=toml_readkey_dbl(toml_tab, "Cal_Offset_BZ", &g_config.offset.z)))
		toml_clean_exit(ret, toml_fp, toml_conf);

	if((ret=toml_readkey_dbl(toml_tab, "Cal_Orth_XY", &g_config.orth.xy)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	if((ret=toml_readkey_dbl(toml_tab, "Cal_Orth_XZ", &g_config.orth.xz)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	if((ret=toml_readkey_dbl(toml_tab, "Cal_Orth_YZ", &g_config.orth.yz)))
		toml_clean_exit(ret, toml_fp, toml_conf);

	if((ret=toml_readkey_dbl(toml_tab, "Cal_Scale_TE", &g_config.scale.temp_e)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	if((ret=toml_readkey_dbl(toml_tab, "Cal_Scale_TS", &g_config.scale.temp_s)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	if((ret=toml_readkey_dbl(toml_tab, "Cal_Scale_TiltX", &g_config.scale.tilt_x)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	if((ret=toml_readkey_dbl(toml_tab, "Cal_Scale_TiltY", &g_config.scale.tilt_y)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	if((ret=toml_readkey_dbl(toml_tab, "Cal_Scale_V5p", &g_config.scale.V5p)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	if((ret=toml_readkey_dbl(toml_tab, "Cal_Scale_V5n", &g_config.scale.V5n)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	if((ret=toml_readkey_dbl(toml_tab, "Cal_Scale_3V3", &g_config.scale.V33)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	if((ret=toml_readkey_dbl(toml_tab, "Cal_Scale_1V5", &g_config.scale.V15)))
		toml_clean_exit(ret, toml_fp, toml_conf);

	if((ret=toml_readkey_dbl(toml_tab, "Cal_Offset_TE", &g_config.offset.temp_e)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	if((ret=toml_readkey_dbl(toml_tab, "Cal_Offset_TS", &g_config.offset.temp_s)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	if((ret=toml_readkey_dbl(toml_tab, "Cal_Offset_TiltX", &g_config.offset.tilt_x)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	if((ret=toml_readkey_dbl(toml_tab, "Cal_Offset_TiltY", &g_config.offset.tilt_y)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	if((ret=toml_readkey_dbl(toml_tab, "Cal_Offset_V5p", &g_config.offset.V5p)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	if((ret=toml_readkey_dbl(toml_tab, "Cal_Offset_V5n", &g_config.offset.V5n)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	if((ret=toml_readkey_dbl(toml_tab, "Cal_Offset_3V3", &g_config.offset.V33)))
		toml_clean_exit(ret, toml_fp, toml_conf);
	if((ret=toml_readkey_dbl(toml_tab, "Cal_Offset_1V5", &g_config.offset.V15)))
		toml_clean_exit(ret, toml_fp, toml_conf);

	/********************************************************************************/
	/* 2nd Block: keys, which depend on keys read in previous block                 */
	/********************************************************************************/
	// locate the [foo] table

	/********************************************************************************/
	/* 3nd Block: keys, which depend on keys read in previous block                 */
	/********************************************************************************/
	// locate the [foo] table

	/* done with toml_conf */
	toml_free(toml_conf);
	toml_conf = NULL;

	return NO_ERROR;
}
