#include "nRF24.h"
#include "spi.h"
#include "usart.h"
#include <stdint.h>
#include <stdio.h>

#define RCC_APB2ENR   (*(volatile uint32_t*)0x40021018)
#define GPIOB_CRL     (*(volatile uint32_t*)0x40010C00)
#define GPIOB_ODR     (*(volatile uint32_t*)0x40010C0C)

void nRF24_Init(void) {
	uint8_t address[] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};

	RCC_APB2ENR |= (1 << 3); // Enable clock for GPIOB
	GPIOB_CRL &= ~(0xF); // Clear pin 0
	GPIOB_CRL |= (0x3); // Set pin 0 to general purpose push pull output
	nRF24_CE_Low(); // Set CE low to start
	nRF24_WriteReg(0x00, 0x08 | 0x03); // Power and set nRF to RX mode (OR with 0x08 to not disrupt EN_CRC)
	for(int i = 0; i < 55000; i++); // Wait ~1.5ms for power up
	nRF24_WriteReg(0x05, 0x4C); // Sets radio to channel 76

	nRF24_WriteRegMulti(0x0A, address, 5); // Sets 5 byte address for RX_ADDR_P0
	nRF24_WriteReg(0x11, 0x04); // Sets length for RX_PW_P0
}

void nRF24_WriteReg(uint8_t addr, uint8_t byte) {
	// Write sequence
	SPI_CS_Low();
	SPI_Transfer(addr | 0x20);
	SPI_Transfer(byte);
	SPI_CS_High();
}

void nRF24_WriteRegMulti(uint8_t addr, uint8_t *bytes, uint8_t len) {
	// Write sequence
	SPI_CS_Low();
	SPI_Transfer(addr | 0x20);
	for(int i = 0; i < len; i++) {
		SPI_Transfer(bytes[i]); // Write len number of bytes through burst write
	}
	SPI_CS_High();
}

uint8_t nRF24_ReadReg(uint8_t addr) {
	uint8_t read; // Read value

	// Read sequence
	SPI_CS_Low();
	SPI_Transfer(addr);
	read = SPI_Transfer(0xFF);
	SPI_CS_High();

	return read; // Return read value
}

void nRF24_ReadPayload(uint8_t *bytes, uint8_t len) {
	// Write sequence
	SPI_CS_Low();
	SPI_Transfer(0x61);
	for(int i = 0; i < len; i++) {
		bytes[i] = SPI_Transfer(0xFF); // Write len number of bytes through burst write
	}
	SPI_CS_High();
	nRF24_WriteReg(0x07, 0x01 << 6); // Clear status register to read again
}

void nRF24_CE_Low(void) {
	GPIOB_ODR &= ~(1); // Clear pin 0 of ODR to set CE low
}

void nRF24_CE_High(void) {
	GPIOB_ODR |= (1); // Set pin 0 of ODR to set CE high
}
