#ifndef DATA_PROCESSING_H_
#define DATA_PROCESSING_H_

#include <stdint.h>
#include "packet.h"

#define AVERAGING 0
#define DECIMATION 1
enum timestamp_pos
{
	TIMESTAMP_AT_FIRST_SAMPLE,
	TIMESTAMP_AT_CENTER,
	TIMESTAMP_AT_LAST_SAMPLE
};

struct bfield_data
{
	double t;
	double bx;
	double by;
	double bz;
	union
	{
		struct
		{
			uint8_t sync : 1;
			uint8_t clip_x : 1;
			uint8_t clip_y : 1;
			uint8_t clip_z : 1;
		} bits;
		uint8_t raw;
	} status;
	uint8_t updated;
};

struct bfield_accu_t
{
	double t_first;
	double t_last;
	double t_center;

	double bx;
	double by;
	double bz;

	uint32_t count;
};

struct hk_data
{
	uint8_t firmware_version;

	double t;

	union
	{
		struct
		{
			uint16_t start : 1;
			uint16_t excit_off : 1;
			uint16_t recording : 1;
			uint16_t flash_recording_disabled : 1;
			uint16_t ol : 1;
			uint16_t peo_s1 : 1;
			uint16_t reserved : 1;
			uint16_t sync_off : 1;
			uint16_t sync_src : 1;
			uint16_t crc_ok : 1;
			uint16_t boot_main : 1;
			uint16_t eeprom_page : 2;
			uint16_t rate : 2;
			uint16_t debug : 1;
		} bits;
		uint16_t raw;
	} status;
	double temp_e;
	double temp_s;
	double tilt_x;
	double tilt_y;
	double V5p;
	double V5n;
	double V33;
	double V15;
	uint32_t rd_ptr;
	uint32_t wr_ptr;
	uint8_t updated;
};

struct hk_accu_t
{
	double t_first;
	double t_last;
	double t_center;

	double temp_e;
	double temp_s;
	double tilt_x;
	double tilt_y;
	double V5p;
	double V5n;
	double V33;
	double V15;

	uint32_t count;
};

int process_cmd_reply_packet(uint8_t* packet);
int process_HK_packet(struct hk_packet* packet);
int process_bfield_packet(struct bfield_packet* packet);
int process_massmem_packet(uint8_t* packet);
int process_gps_packet(uint8_t* packet);

#endif /* DATA_PROCESSING_H_ */
