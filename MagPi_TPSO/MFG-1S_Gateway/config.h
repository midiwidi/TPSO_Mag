#ifndef CONFIG_H_
#define CONFIG_H_

#include "data_processing.h"

#define VERSION "0.1"	// might be generated from Git version later
#define HEX_BYTES_PER_ROW 48

#define DEFAULT_LOGLEVEL LOG_WARNING

#define DEFAULT_AVG_N_BFIELD 			100		//every second (@ 100Hz Magnetometer output rate)
#define DEFAULT_AVG_N_HK 				10		//every 10s
#define DEFAULT_TELEMETRY_RESOLUTION 	0.007748603821
#define DEFAULT_TIMESTAMP_POSITION		TIMESTAMP_AT_LAST_SAMPLE

#define CMD_FIFO "/tmp/tpsomag_cmd"
#define DAT_FIFO "/tmp/tpsomag_dat"

#define DATA_BUF_DEPTH 		200
#define CMD_BUF_DEPTH  		32

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
};

#endif /* CONFIG_H_ */
