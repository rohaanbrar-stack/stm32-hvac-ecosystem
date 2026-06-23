#include "i2c.h"
#include "bmp280.h"
#include "usart.h"
#include "clock.h"
#include <stdint.h>
#include <stdio.h>

int main(void)
{
	Clock_Init();

	char buffer[40];
	uint32_t adc_T;
	int32_t temp;

	I2C_Init();
	BMP280_Init();
	USART_Init();

	while(1) {
		adc_T = BMP280_ReadTemp();
		temp = BMP280_Compensate(adc_T);
		sprintf(buffer, "%ld\r\n", temp);

		int i = 0;
		while(buffer[i] != '\0') {
			USART_WriteByte(buffer[i]);
			i++;
		}
	}
}
