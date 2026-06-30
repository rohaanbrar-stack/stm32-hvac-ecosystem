#include "i2c.h"
#include "bmp280.h"
#include "usart.h"
#include "clock.h"
#include "servo.h"
#include "spi.h"
#include "nRF24.h"
#include <stdint.h>
#include <stdio.h>

int main(void)
{
	// Driver initializations
	Clock_Init();
	I2C_Init();
	BMP280_Init();
	USART_Init();
	SPI_Init();
	Servo_Init();
	nRF24_Init();

	// Variable declarations
	uint32_t adc_T;
	int32_t temp;
	volatile uint32_t MAX = 5000000;
	uint8_t nRF_Val;
	char buffer[40];
	uint8_t dummy_bytes[] = {0xFF, 0xFF, 0xFF, 0xFF};

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

		//Alter servo based on temperature
		if(temp > 2600) Servo_SetAngle(180, 1);
		else Servo_SetAngle(0, 1);

		// Print temperature to computer via USART
		sprintf(buffer, "%ld\r\n", temp);
		int i = 0;
		while(buffer[i] != '\0') {
			USART_WriteByte(buffer[i]);
			i++;
		}

		// nRF24 test
		for(int i = 0; i < 1000000; i++);
		nRF24_WritePayload(dummy_bytes, 4);
		nRF_Val = nRF24_ReadReg(0x07);
		if((nRF_Val & (0x01 << 4)) != 0) {
			nRF24_WriteReg(0x07, 0x01 << 4);
			nRF24_FlushTX();
		}
		sprintf(buffer, "%02X\r\n", nRF_Val);
		i = 0;
		while(buffer[i] != '\0') {
			USART_WriteByte(buffer[i]);
			i++;
		}
	}
}
