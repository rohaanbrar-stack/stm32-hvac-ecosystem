#include "clock.h"
#include "usart.h"
#include "spi.h"
#include "nRF24.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

void send_frame(uint8_t type, uint8_t *payload, uint8_t len) {
	USART_WriteByte(USART2, 0xAA); // Send start byte
	uint8_t checksum = type; // Add command type to checksum
	USART_WriteByte(USART2, type); // Send command type
	for(int i = 0; i < len; i++) { USART_WriteByte(USART2, payload[i]); checksum += payload[i]; } // Send each payload byte and add to checksum
	USART_WriteByte(USART2, checksum); // Send checksum
}

bool receive_frame(uint8_t *type, uint8_t *payload) {
	while(USART_ReadByte(USART2) != 0xAA); // Poll until start byte is recieved
	*type = USART_ReadByte(USART2); // Read type byte
	uint8_t len;
	switch(*type) {    // Check type byte for payload size
		case 0x01:
			len = 2;
			break;
		case 0x02:
			len = 1;
			break;
		case 0x03:
			len = 1;
			break;
		default:
			return false;
	}
	uint8_t checksum = *type;
	for(int i = 0; i < len; i++) { payload[i] = USART_ReadByte(USART2); checksum += payload[i]; } // Read each payload byte and add to checksum
	uint8_t checkTX = USART_ReadByte(USART2); // Read checksum byte
	if(checksum == checkTX) { return true; } // Compare checksums for corruption
	else return false;
}

int main(void)
{
	volatile uint32_t MAXM = 1000000;

	// Driver initializations
	Clock_Init();
	USART1_Init();
	USART2_Init();
	print_reset_cause();
    SPI_Init();

    // Variable declarations
    char buffer[48];
    uint8_t type;
    uint8_t payload[2];
    int16_t temp;
    uint8_t cmd;

    while(1) {

    	// Read temperature from ceiling node
    	bool conf = receive_frame(&type, payload);

    	// Check for command type
    	if(conf && type == 0x01) { // Temperature
    		temp = (payload[0] << 8) | payload[1];
    		// Print temperature to USART
    		sprintf(buffer, "%d, %d, %d\r\n", type, temp, conf);
    		int i = 0;
    		while(buffer[i] != '\0') { USART_WriteByte(USART1, buffer[i]); i++; }

    		// Send servo controls
    		cmd = (temp > 2750) ? 180:0;
    		send_frame(0x02, &cmd, 1);
    	}
    	else if(conf && type == 0x03) { // ACK
    		sprintf(buffer, "ACK\r\n");
    		int i = 0;
    		while(buffer[i] != '\0') { USART_WriteByte(USART1, buffer[i]); i++; }
    	}
    }
}
