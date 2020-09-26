#include <stddef.h>
#include "globals.h"
#include "log.h"
#include "config.h"

int g_ser_port = -1;
int g_fd_fifocmd = -1;
uint32_t g_packet_error_cnt = 0;
char* g_conf_file = NULL;
uint8_t g_startup = 1;
double g_mag_time = 0;

struct config g_config = {	.avg_N_bfield = DEFAULT_AVG_N_BFIELD,
							.avg_N_hk = DEFAULT_AVG_N_HK,
							.loglevel = DEFAULT_LOGLEVEL,
							.scale = {1, 1, 1,
									  0.01001445,
									  -0.0085630207,
									  -0.000059502559,
									  -0.000059575229,
									  0.0002,
									  0.0004,
									  0.000133,
									  0.0001},
							.offset = {0, 0, 0,
									   -273.12595008,
									   159.61696874,
									   1.26686898,
									   1.26022460,
									   0.0,
									   -3.0,
									   0.0,
									   0.0},
							.orth = {0, 0, 0},
							.telemetry_resolution = DEFAULT_TELEMETRY_RESOLUTION,
                            .timestamp_position = TIMESTAMP_AT_CENTER,
                            .downsample_mode = AVERAGING,
							.serial_port_device = NULL,
							.mag_output_file = NULL,
							.mag_minmax_file = NULL,
							.HK_output_file = NULL,
							.HK_minmax_file = NULL};

struct bfield_data g_bfield_data;
struct hk_data g_hk_data;
