#ifndef PACKET_H_
#define PACKET_H_

#include <stdint.h>

#define GPOL_KERMIT 0x8408 //Generator polynomial for CRC16 (Version Kermit)

#define PACKET_LEN_REPLY	8		// 6 byte cargo + 2 byte CRC
#define PACKET_LEN_HK		34		// 32 byte cargo + 2 byte CRC
#define PACKET_LEN_BFIELD	19		// 17 byte cargo + 2 byte CRC
#define PACKET_LEN_MASSMEM	2051	// 2049 byte cargo + 2 byte CRC
#define PACKET_LEN_GPS		30		// 28 byte cargo + 2 byte CRC

#define PACKET_ID_REPLY		0xAA
#define PACKET_ID_HK		0xAB
#define PACKET_ID_BFIELD	0xAC
#define PACKET_ID_MASSMEM	0xAD
#define PACKET_ID_GPS		0xAE

#pragma pack(1)
struct bfield_packet
{
		uint8_t packet_ID;
		uint16_t subsec;
		uint32_t sec;
		uint8_t bx0;
		uint8_t bx1;
		uint8_t bx2;
		uint8_t by0;
		uint8_t by1;
		uint8_t by2;
		uint8_t bz0;
		uint8_t bz1;
		uint8_t bz2;
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
};

struct hk_packet
{
		uint8_t packet_ID;
		uint8_t firmware_version;
		uint16_t subsec;
		uint32_t sec;
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
		uint16_t temp_e;
		uint16_t temp_s;
		uint16_t tilt_x;
		uint16_t tilt_y;
		uint16_t V5p;
		uint16_t V5n;
		uint16_t V33;
		uint16_t V15;
		uint8_t rd_ptr0;
		uint8_t rd_ptr1;
		uint8_t rd_ptr2;
		uint8_t wr_ptr0;
		uint8_t wr_ptr1;
		uint8_t wr_ptr2;
};
#pragma pack()


int packet_decode(uint8_t* packet, uint8_t packet_len);
uint16_t CheckCRC16(uint8_t *cargo, uint16_t length);

#endif /* PACKET_H_ */
