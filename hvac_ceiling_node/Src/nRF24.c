#include "nRF24.h"
#include "spi.h"
#include <stdint.h>

uint8_t nRF24_ReadReg(uint8_t addr) {
	uint8_t read; // Read value

	// Read sequence
	SPI_CS_Low();
	SPI_Transfer(addr);
	read = SPI_Transfer(0xFF);
	SPI_CS_High();

	return read; // Return read value
}
