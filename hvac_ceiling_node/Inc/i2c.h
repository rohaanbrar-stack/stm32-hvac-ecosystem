#ifndef I2C_H
#define I2C_H

#include <stdint.h>

void I2C_Init(void);
void I2C_Start(void);
void I2C_WriteAddress(uint8_t addr, uint8_t rw);
void I2C_WriteByte(uint8_t byte);
uint8_t I2C_ReadByte(uint8_t ack);
void I2C_Stop(void);

#endif
