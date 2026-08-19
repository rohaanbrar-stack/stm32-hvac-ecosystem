#include "sd.h"
#include "spi.h"
#include <stdint.h>
#include <stdio.h>

volatile uint8_t last_cmd24_response;
volatile uint8_t write_response;

void SD_SendCommand(uint8_t cmd, uint32_t arg, uint8_t crc) {
	// Arg byte splitting
	uint8_t byte_1 = (arg >> 24) & 0xFF;
	uint8_t byte_2 = (arg >> 16) & 0xFF;
	uint8_t byte_3 = (arg >> 8) & 0xFF;
	uint8_t byte_4 = arg & 0xFF;

	SPI_Transfer(0x40 | cmd); // Send command line
	SPI_Transfer(byte_1); // Send command arg
	SPI_Transfer(byte_2); //
	SPI_Transfer(byte_3); //
	SPI_Transfer(byte_4); //
	SPI_Transfer(crc); // Send CRC
	for(int i = 0; i < 100000; i++) if(!(SPI1_SR & (1 << 7))) break; // Waits until status bit is set by hardware
}

uint8_t SD_ReadResponse() {
	uint8_t byte;
	// Poll until data byte is returned
	for(int i = 0; i < 100000; i++) {
		byte = SPI_Transfer(0xFF);
		if(byte != 0xFF) {
			return byte;
		}
	}
	return 0xFF; // Timeout
}

uint8_t SD_Init(void) {
	// Send 10 bytes to wake SD up
	SPI_CS_High();
	for(int i = 0; i < 80; i++) SPI_Transfer(0xFF);

	// Send CMD0
	SPI_CS_Low();
	SD_SendCommand(0, 0x00000000, 0x95);
	uint8_t response = SD_ReadResponse();
	if(response != 0x01) {
		SPI_CS_High();
		return 0x01; // CMD0 failed, card not responding in SPI mode
	}
	SPI_Transfer(0xFF);
	SPI_CS_High();

	// Send CMD8
	SPI_CS_Low();
	SD_SendCommand(8, 0x000001AA, 0x87);
	response = SD_ReadResponse();

	// CMD8 not supported, use legacy init
	if(response != 0x01) {
		SPI_Transfer(0xFF);
		SPI_CS_High();

		// Send CMD55 and ACMD41 until response is 0x00
		for(int i = 0; i < 100000; i++) {
			SPI_CS_Low();
			SD_SendCommand(55, 0x00000000, 0xFF);
			response = SD_ReadResponse();
			SPI_Transfer(0xFF);
			SPI_CS_High();
			SPI_CS_Low();
			SD_SendCommand(41, 0x00000000, 0xFF);
			response = SD_ReadResponse();
			SPI_Transfer(0xFF);
			SPI_CS_High();
			SPI_Transfer(0xFF);
			if(response == 0x00) return 0x00;
		}
		return 0x03; // SDSC v1 ACMD41 timed out, card stuck in idle
	}
	(void)SD_ReadResponse();
	(void)SD_ReadResponse();
	(void)SD_ReadResponse();
	(void)SD_ReadResponse();
	SPI_Transfer(0xFF);
	SPI_CS_High();

	// Send CMD55 and ACMD41 until response is 0x00
	for(int i = 0; i < 100000; i++) {
		SPI_CS_Low();
		SD_SendCommand(55, 0x00000000, 0xFF);
		response = SD_ReadResponse();
		SPI_Transfer(0xFF);
		SPI_CS_High();
		SPI_CS_Low();
		SD_SendCommand(41, 0x40000000, 0xFF);
		response = SD_ReadResponse();
		SPI_Transfer(0xFF);
		SPI_CS_High();
		SPI_Transfer(0xFF);
		if(response == 0x00) return 0x00;
	}
	return 0x04; // SDHC ACMD41 timed out, card stuck in idle
}

uint8_t SD_WriteBlock(uint32_t block, uint8_t *data) {
	// Initialize block address
	SPI_CS_Low();
	int i;
	uint8_t response;
	for(i = 0; i < 100000; i++) {
		SD_SendCommand(24, block*512, 0xFF);
		response = SD_ReadResponse();
		if(response == 0x00) break;
	}
	if(i == 100000) {
		last_cmd24_response = response;
		SPI_CS_High();
		return 0x04; // CMD24 never accepted, card not ready to write
	}
	SPI_Transfer(0xFF);

	// Transfer data to block
	SPI_Transfer(0xFE);
	for(int i = 0; i < 512; i++) SPI_Transfer(data[i]);
	SPI_Transfer(0xFF);
	SPI_Transfer(0xFF);

	// Check data was accepted
	write_response = SD_ReadResponse();
	if((write_response & 0x0E) != 0x04) {
		SPI_CS_High();
		return 0x01; // Data token rejected, write not accepted by card
	}
	for(int i = 0; i < 1000; i++) {
		if(SPI_Transfer(0xFF) != 0x00) break;
		else if(i == 999) {
			SPI_CS_High();
			return 0x02; // Card busy timeout after write, never released MISO
		}
	}
	SPI_Transfer(0xFF);
	SPI_CS_High();
	return 0x00;
}

uint8_t SD_ReadBlock(uint32_t block, uint8_t *data) {
	// Initialize block address
	SPI_CS_Low();
	SD_SendCommand(17, block*512, 0xFF);
	uint8_t response = SD_ReadResponse();
	if(response != 0x00) {
		SPI_CS_High();
		return 0x01; // CMD17 failed, card rejected read command
	}

	// Transfer data from block
	for(int i = 0; i < 100000; i++) {
		if(SPI_Transfer(0xFF) == 0xFE) break;
		else if(i == 99999) {
			SPI_CS_High();
			return 0x02; // Data token never received, card didn't send data
		}
	}
	for(int i = 0; i < 512; i++) data[i] = SPI_Transfer(0xFF);
	SPI_Transfer(0xFF);
	SPI_Transfer(0xFF);
	SPI_CS_High();
	return 0x00;
}

