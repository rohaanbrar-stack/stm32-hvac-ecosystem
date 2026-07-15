#include "i2c.h"
#include "bmp280.h"
#include "usart.h"
#include "clock.h"
#include "servo.h"
#include "spi.h"
#include "nRF24.h"
#include <stdint.h>
#include <stdio.h>

#define GPIOB_ODR     (*(volatile uint32_t*)0x40010C0C)

int main(void)
{
	volatile uint32_t MAXM = 1000000;
	char buffer[48];

	// Driver initializations
	Clock_Init();
	USART_Init();
	print_reset_cause();
	sprintf(buffer, "ALIVE5\r\n");
	int i = 0;
	while(buffer[i] != '\0') { USART_WriteByte(buffer[i]); i++; }
	I2C_Init();
	sprintf(buffer, "I2C\r\n");
		i = 0;
		while(buffer[i] != '\0') { USART_WriteByte(buffer[i]); i++; }
	BMP280_Init();
	sprintf(buffer, "BMP\r\n");
		i = 0;
		while(buffer[i] != '\0') { USART_WriteByte(buffer[i]); i++; }
	SPI_Init();
	Servo_Init();
	for(int i = 0; i < MAXM; i++);
	sprintf(buffer, "NRF\r\n");
	i = 0;
	while(buffer[i] != '\0') { USART_WriteByte(buffer[i]); i++; }
	nRF24_Init();
	nRF24_Dump();

	// Variable declarations
	uint32_t adc_T;
	int32_t temp;
	volatile uint32_t MAX = 5000000;

	uint8_t nRF_Val;

	uint8_t dummy_bytes[] = {0xDE, 0xAD, 0xBE, 0xEF};

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
		for(int i = 0; i < MAXM; i++);
		nRF24_WritePayload(dummy_bytes, 4);
		for(int j = 0; j < 1000; j++) {
		    nRF_Val = nRF24_ReadReg(0x07);
		    if((nRF_Val & (0x03 << 4)) != 0) {
		        break;
		    }
		}
		uint8_t obs  = nRF24_ReadReg(0x08);   // read BEFORE clearing/flushing
		uint8_t fifo = nRF24_ReadReg(0x17);
		sprintf(buffer, "ST:%02X OBS:%02X PLOS:%d ARC:%d FIFO:%02X\r\n",
		        nRF_Val, obs, obs >> 4, obs & 0x0F, fifo);
		i = 0;
		while(buffer[i] != '\0') { USART_WriteByte(buffer[i]); i++; }

		nRF24_WriteReg(0x07, (1<<4) | (1<<5));   // clear MAX_RT + TX_DS, every time
		nRF24_FlushTX();                          // drop any stuck packet, every time
		i = 0;
		while(buffer[i] != '\0') {
			USART_WriteByte(buffer[i]);
			i++;
		}
	}
}
