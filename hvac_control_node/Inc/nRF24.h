#ifndef NRF24_H
#define NRF24_H

#include <stdint.h>


void print_reset_cause(void);
void nRF24_WriteRegVerified(uint8_t, uint8_t);
void nRF24_Init(void);
void nRF24_Dump(void);
uint8_t nRF24_ReadReg(uint8_t);
void nRF24_ReadRegMulti(uint8_t, uint8_t*, uint8_t);
void nRF24_WriteReg(uint8_t, uint8_t);
void nRF24_WriteRegMulti(uint8_t, uint8_t*, uint8_t);
void nRF24_ReadPayload(uint8_t*, uint8_t);
void nRF24_CE_Low(void);
void nRF24_CE_High(void);

#endif
