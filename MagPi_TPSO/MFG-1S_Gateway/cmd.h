#ifndef CMD_H_
#define CMD_H_

#include <stdint.h>

#define CMD_LENGTH 6	// 4 Bytes command + 2 Bytes CRC

typedef struct {
   uint8_t MAG_CFG_REG;
   uint8_t PHASE_X;
   uint8_t PHASE_Y;
   uint8_t PHASE_Z;
   uint8_t NOSP;
   uint8_t K1X;
   uint8_t K1Y;
   uint8_t K1Z;
   uint8_t K2X;
   uint8_t K2Y;
   uint8_t K2Z;
   uint8_t K3X;
   uint8_t K3Y;
   uint8_t K3Z;
   uint8_t KF;
   uint8_t DACFACTX;
   uint8_t DACFACTY;
   uint8_t DACFACTZ;
   uint8_t RATE;

   uint8_t DACF_X;
   uint8_t DACF_Y;
   uint8_t DACF_Z;
   uint8_t DACC_X;
   uint8_t DACC_Y;
   uint8_t DACC_Z;
   uint8_t PRAM_MIRR_DATA;
   uint8_t PRAM_MIRR_ADDR;
   uint8_t BOOTOP;
   uint8_t MASSMEM_CMD_ID;
   uint8_t MASSMEM_CMD_DATA;
   uint8_t DRAM_ADDR;
   uint8_t DRAM_DATA_MSW;
   uint8_t DRAM_DATA_LSW;
   uint8_t FLUSH_BUFF_TO_MASSMEM;
   uint8_t TIMESTAMP_MSW;
   uint8_t TIMESTAMP_LSW;
   uint8_t BAUD_RATE_DIV;
} cmd_id_t;
extern const cmd_id_t cmd;

#define RESET_BIT	0x100

#define APP_CMD_AVG_SAMP_BFIELD		0
#define APP_CMD_AVG_SAMP_HK			1
#define APP_CMD_LOGLEVEL			2
#define APP_CMD_READ_CFG			3
#define APP_SET_TIME				4
#define APP_CMD_EXIT				128

int send_cmd(uint8_t cmd_id, uint32_t cmd_data);


#endif /* CMD_H_ */
