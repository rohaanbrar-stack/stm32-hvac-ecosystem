#include "i2c.h"
#include "bmp280.h"
#include "usart.h"
#include "clock.h"
#include "servo.h"
#include "timer.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define FRAME_START 150
#define FRAME_BYTE 5

void send_frame(uint8_t type, uint8_t *payload, uint8_t len) {
	USART_WriteByte(USART2, 0xAA); // Send start byte
	uint8_t checksum = type; // Add command type to checksum
	USART_WriteByte(USART2, type); // Send command type
	for(int i = 0; i < len; i++) { USART_WriteByte(USART2, payload[i]); checksum += payload[i]; } // Send each payload byte and add to checksum
	USART_WriteByte(USART2, checksum); // Send checksum
}

bool receive_frame(uint8_t *type, uint8_t *payload) {
	uint8_t byte;
	do {
		if(!USART_ReadByte(USART2, &byte, FRAME_START)) return false;
	} while(byte != 0xAA); // Poll until start byte is received
	if(!USART_ReadByte(USART2, type, FRAME_BYTE)) return false; // Read type byte
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
	for(int i = 0; i < len; i++) {if(!USART_ReadByte(USART2, &payload[i], FRAME_BYTE)) return false; checksum += payload[i]; } // Read each payload byte and add to checksum
	uint8_t checkTX;
	if(!USART_ReadByte(USART2, &checkTX, FRAME_BYTE)) return false; // Read checksum byte
	if(checksum == checkTX) { return true; } // Compare checksums for corruption
	else return false;
}

int main(void)
{
	char buffer[48];

	// Driver initializations
	Clock_Init();
	TIM3_Init();
	USART1_Init();
	USART2_Init();
	sprintf(buffer, "ALIVE5\r\n");
	int i = 0;
	while(buffer[i] != '\0') { USART_WriteByte(USART1, buffer[i]); i++; }
	I2C_Init();
	sprintf(buffer, "I2C\r\n");
	i = 0;
	while(buffer[i] != '\0') { USART_WriteByte(USART1, buffer[i]); i++; }
	BMP280_Init();
	sprintf(buffer, "BMP\r\n");
	i = 0;
	while(buffer[i] != '\0') { USART_WriteByte(USART1, buffer[i]); i++; }
	Servo_Init();

	// Variable declarations
	uint32_t adc_T;
	int32_t temp;
	uint8_t type;
	uint8_t cmd;

	// Main loop
	while(1) {
		// Take temperature measurement
		adc_T = BMP280_ReadTemp();
		temp = BMP280_Compensate(adc_T);

		// Print temperature to computer via USART
		sprintf(buffer, "%ld, %lu\r\n", temp, timestamp);
		int i = 0;
		while(buffer[i] != '\0') {
			USART_WriteByte(USART1, buffer[i]);
			i++;
		}

		// Send temperature to control node
		uint8_t payload[2] = {(temp >> 8) & (0xFF), temp & 0xFF};
		send_frame(0x01, payload, 2);

		// Latch start timestamp for 1 second
		uint32_t start = timestamp;

		// One-second delay
		while(timestamp - start < 100) {
			//Alter servo based on control command
			if (USART_DataAvailable(USART2)) {   // Check if byte is waiting
				if(receive_frame(&type, &cmd)) {
					if (type == 0x02) { // Run servo commands
						Servo_SetAngle(cmd, 1);
						Servo_SetAngle(cmd, 2);
						send_frame(0x03, &cmd, 1);
						sprintf(buffer, "CMD: %d\r\n", cmd);
						int j = 0;
						while(buffer[j] != '\0') { USART_WriteByte(USART1, buffer[j]); j++; }
					}
				}
			}
		}
	}
}
