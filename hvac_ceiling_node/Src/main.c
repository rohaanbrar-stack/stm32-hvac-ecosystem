#include "i2c.h"
#include "bmp280.h"
#include "usart.h"
#include "clock.h"
#include "servo.h"
#include <stdint.h>
#include <stdio.h>

int main(void)
{
	// Driver initializations
	Clock_Init();
	I2C_Init();
	BMP280_Init();
	USART_Init();
	Servo_Init();

	// Variable declarations
	char buffer[40];
	uint32_t adc_T;
	int32_t temp;
	volatile uint32_t MAX = 5000000;

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
	}
}
