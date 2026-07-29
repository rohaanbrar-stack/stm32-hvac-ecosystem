#include "clock.h"
#include "bmp280.h"
#include "i2c.h"
#include "usart.h"
#include "spi.h"
#include "nRF24.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define SET 2556
#define D 85
#define M 110

typedef enum {CLOSED, OPEN_COOLING, OPEN_HEATING} ventState;

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
	// Driver initializations
	Clock_Init();
	USART1_Init();
	USART2_Init();
	I2C_Init();
	print_reset_cause();
	BMP280_Init();
    SPI_Init();

    // Variable declarations
    char buffer[48];
    uint8_t type;
    uint8_t payload[2];
    uint32_t adc_T;
    int32_t temp;
    int16_t tempC;
    uint8_t cmd;
    ventState state = CLOSED;

    while(1) {

    	// Take temperature measurement
    	adc_T = BMP280_ReadTemp();
    	temp = BMP280_Compensate(adc_T);

    	// Print temperature to computer via USART
    	sprintf(buffer, "%ld\r\n", temp);
    	int i = 0;
    	while(buffer[i] != '\0') {
    		USART_WriteByte(USART1, buffer[i]);
    		i++;
    	}

    	// Read temperature from ceiling node
    	bool conf = receive_frame(&type, payload);

    	// Check for command type
    	if(conf && type == 0x01) { // Temperature
    		tempC = (payload[0] << 8) | payload[1];
    		// Print temperature to USART
    		sprintf(buffer, "%ld, %d, %d, %d, %d\r\n", temp, type, tempC, conf, state);
    		int i = 0;
    		while(buffer[i] != '\0') { USART_WriteByte(USART1, buffer[i]); i++; }

    		// Send servo controls
    		switch(state) {    // Check current vent state
    			case CLOSED:
    				if((temp > (SET + D)) && (tempC < (temp - M))) {cmd = 180; send_frame(0x02, &cmd, 1); state = OPEN_COOLING; break;} // Open duct for cooling
    				else if((temp < (SET - D)) && (tempC > (temp + M))) {cmd = 180; send_frame(0x02, &cmd, 1); state = OPEN_HEATING; break;} // Open duct for heating
    				else break;
    			case OPEN_COOLING:
    				if(temp <= SET) {cmd = 0; send_frame(0x02, &cmd, 1); state = CLOSED; break;} // Close duct if room temperature is right
    				else if(tempC >= temp) {cmd = 0; send_frame(0x02, &cmd, 1); state = CLOSED; break;} // Close duct if duct is hot when it should be cold
    				else break;
    			case OPEN_HEATING:
    				if(temp >= SET) {cmd = 0; send_frame(0x02, &cmd, 1); state = CLOSED; break;} // Close duct if room temperature is right
    				else if(tempC <= temp) {cmd = 0; send_frame(0x02, &cmd, 1); state = CLOSED; break;} // Close duct if duct is cold when it should be hot
    				else break;
    			default:
    				break;
    		}
    	}
    	else if(conf && type == 0x03) { // ACK
    		sprintf(buffer, "ACK\r\n");
    		int i = 0;
    		while(buffer[i] != '\0') { USART_WriteByte(USART1, buffer[i]); i++; }
    	}
    }
}
