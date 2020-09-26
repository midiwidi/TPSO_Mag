#ifndef GLOBALS_H_
#define GLOBALS_H_

#include <stdint.h>
#include "data_processing.h"
#include "config.h"

extern int g_ser_port;
extern int g_fd_fifocmd;
extern uint32_t g_packet_error_cnt;
extern char* g_conf_file;
extern uint8_t g_startup;
extern double g_mag_time;

extern struct config g_config;
extern struct bfield_data g_bfield_data;
extern struct hk_data g_hk_data;

#endif /* GLOBALS_H_ */
