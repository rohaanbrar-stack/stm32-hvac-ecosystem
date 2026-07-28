#include "i2c.h"
#include "bmp280.h"
#include "usart.h"
#include "clock.h"
#include "servo.h"
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
	char buffer[48];

	// Driver initializations
	Clock_Init();
	USART1_Init();
	USART2_Init();
	print_reset_cause();
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
	SPI_Init();
	Servo_Init();

	// Variable declarations
	uint32_t adc_T;
	int32_t temp;
	uint8_t type;
	uint8_t cmd;
	volatile uint32_t MAX = 5000000;

	for(int i = 0; i < MAXM; i++);

	// Servo test
	Servo_SetAngle(0, 1);
	Servo_SetAngle(0, 2);
	for(int i = 0; i < MAX; i++);
	Servo_SetAngle(180, 1);
	Servo_SetAngle(180, 2);
	for(int i = 0; i < MAX; i++);
	Servo_SetAngle(45, 1);
	Servo_SetAngle(45, 2);
	for(int i = 0; i < MAX; i++);
	Servo_SetAngle(135, 1);
	Servo_SetAngle(135, 2);
	for(int i = 0; i < MAX; i++);
	Servo_SetAngle(90, 1);
	Servo_SetAngle(90, 2);
	for(int i = 0; i < MAX; i++);

	// Main loop
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

		// Send temperature to control node
		uint8_t payload[2] = {(temp >> 8) & (0xFF), temp & 0xFF};
		send_frame(0x01, payload, 2);

		// One-second delay
		for(int i = 0; i < MAX; i++) {
			//Alter servo based on control command
			if (USART_DataAvailable(USART2)) {   // Check if byte is waiting
				if(receive_frame(&type, &cmd)) {
					if (type == 0x02) {
						Servo_SetAngle(cmd, 1);
						Servo_SetAngle(cmd, 2);
						send_frame(0x03, &cmd, 1);
						sprintf(buffer, "CMD: %d\r\n", cmd);
						i = 0;
						while(buffer[i] != '\0') { USART_WriteByte(USART1, buffer[i]); i++; }
					}
				}
			}
		}
	}
}
