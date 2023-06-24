#ifndef CONFIG_H_
#define CONFIG_H_

#include "data_processing.h"

#define CONSOLE_OUT 1
#define HEX_BYTES_PER_ROW 48

#define DEFAULT_LOGLEVEL LOG_WARNING
#define DEFAULT_AVG_N_BFIELD 				6000		//every minute (@ 100Hz Magnetometer output rate)
#define DEFAULT_AVG_N_HK 					240			//every 4min
#define DEFAULT_TELEMETRY_RESOLUTION 		0.007748603821
#define DEFAULT_TIMESTAMP_POSITION			TIMESTAMP_AT_LAST_SAMPLE
#define DEFAULT_LOOP_SLEEP_TIME				1000
#define NO_OUTPUT_FOR_N_SAMP_AFTER_TIMESET	2
#define NULLS_DELTA_TSUBSEC					1

#define CMD_FIFO "/tmp/tpsomag_cmd"
#define DAT_FIFO "/tmp/tpsomag_dat"

#define DATA_BUF_DEPTH 		200
#define CMD_BUF_DEPTH  		32

#define MIN_VALID_TIME		1577836800	// 2020-01-01 00:00:00 (in sec since 01.01.1970)

struct config
{
	uint32_t avg_N_bfield;
	uint32_t avg_N_hk;
	int loglevel;
	struct
	{
		double x;
		double y;
		double z;
		double temp_s;
		double temp_e;
		double tilt_x;
		double tilt_y;
		double V5p;
		double V5n;
		double V33;
		double V15;
	} scale;
	struct
	{
		double x;
		double y;
		double z;
		double temp_s;
		double temp_e;
		double tilt_x;
		double tilt_y;
		double V5p;
		double V5n;
		double V33;
		double V15;
	} offset;
	struct
	{
		double xy;
		double xz;
		double yz;
	} orth;
	double telemetry_resolution;
	enum timestamp_pos timestamp_position;
	uint8_t downsample_mode;

	char* serial_port_device;
	char* mag_output_file;
	char* mag_minmax_file;
	char* HK_output_file;
	char* HK_minmax_file;
};

#endif /* CONFIG_H_ */
