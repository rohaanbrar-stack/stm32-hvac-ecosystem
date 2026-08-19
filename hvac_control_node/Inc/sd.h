#ifndef SD_H
#define SD_H

#include <stdint.h>

uint8_t SD_Init(void);
uint8_t SD_WriteBlock(uint32_t block, uint8_t *data);
uint8_t SD_ReadBlock(uint32_t block, uint8_t *data);
extern volatile uint8_t last_cmd24_response;
extern volatile uint8_t write_response;

#endif
