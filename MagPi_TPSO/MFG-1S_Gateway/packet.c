#include <stdint.h>
#include "packet.h"
#include "helpers.h"
#include "log.h"
#include "globals.h"
#include "data_processing.h"

int packet_decode(uint8_t* packet, uint8_t packet_len)
{
	unsigned int crc;

	if (packet_len < 2)
	{
		log_write(LOG_WARNING, "packet too short (%d byte)", packet_len);
		return ERROR;
	}

	crc = CheckCRC16(packet, packet_len - 2);
	if (crc != ((packet[packet_len-1]<<8) | packet[packet_len-2]) )
	{
		log_write(LOG_WARNING, "CRC error, packet dropped, calc=0x%04x, packet=0x%04x", crc, ((packet[packet_len-1]<<8) | packet[packet_len-2]));
		return ERROR;
	}

	switch (packet[0]) //Check Packet ID
	{
		case PACKET_ID_REPLY:	//CMD Reply Packet
			if (packet_len == PACKET_LEN_REPLY)
				process_cmd_reply_packet(packet);
			else
			{
				log_write(LOG_WARNING, "wrong \"CMD reply\" packet length - expected: %d, actual: %d", PACKET_LEN_REPLY, packet_len);
				return ERROR;
			}
			break;
		case PACKET_ID_HK:	//HK Packet
			if (packet_len == PACKET_LEN_HK)
				process_HK_packet((struct hk_packet*)packet);
			else
			{
				log_write(LOG_WARNING, "wrong HK packet length - expected: %d, actual: %d", PACKET_LEN_HK, packet_len);
				return ERROR;
			}
			break;
		case PACKET_ID_BFIELD:	//Magnetic Field Packet
			if (packet_len == PACKET_LEN_BFIELD)
				process_bfield_packet((struct bfield_packet*)packet);
			else
			{
				log_write(LOG_WARNING, "wrong B-field packet length - expected: %d, actual: %d", PACKET_LEN_BFIELD, packet_len);
				return ERROR;
			}
			break;
		case PACKET_ID_MASSMEM:   //Massmem Frame
			if (packet_len == PACKET_LEN_MASSMEM)
				process_massmem_packet(packet);
			else
			{
				log_write(LOG_WARNING, "wrong mass memory packet length - expected: %d, actual: %d", PACKET_LEN_MASSMEM, packet_len);
				return ERROR;
			}
			break;
		case PACKET_ID_GPS:   //GPS Frame
			if (packet_len == PACKET_LEN_GPS)
				process_gps_packet(packet);
			else
			{
				log_write(LOG_WARNING, "wrong GPS packet length - expected: %d, actual: %d", PACKET_LEN_GPS, packet_len);
				return ERROR;
			}
			break;
		default:
			log_write(LOG_WARNING, "dropped packet with unknown ID (%d)", packet[0]);
			return ERROR;
			break;
	}

	return NO_ERROR;
}

//CRC16 Calculation
uint16_t CheckCRC16(uint8_t *cargo, uint16_t length)
{
	uint8_t j,droppedbit;
	uint16_t i,checksum;

	checksum=0;

	for(i=0;i<length;i++)
	{

		checksum = checksum ^ (0xFF&*(cargo+i));
		for(j=0;j<8;j++)
		{
			droppedbit = checksum & 1;
			checksum = checksum>>1;
			if(1 == droppedbit)
			{
				checksum = checksum ^ GPOL_KERMIT;
			}
		}

	}

	// swap LSByte und MSByte
	return (((checksum&0xFF)<<8)|((checksum>>8)&0xFF));
}
