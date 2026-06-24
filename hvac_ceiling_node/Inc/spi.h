#ifndef SPI_H
#define SPI_H
#define SPI1_CR1      (*(volatile uint32_t*)0x40013000)
#define SPI1_SR       (*(volatile uint32_t*)0x40013008)
#define SPI1_DR       (*(volatile uint32_t*)0x4001300C)

#include <stdint.h>

void SPI_Init(void);
uint8_t SPI_Transfer(uint8_t data);
void SPI_CS_Low(void);
void SPI_CS_High(void);

#endif
