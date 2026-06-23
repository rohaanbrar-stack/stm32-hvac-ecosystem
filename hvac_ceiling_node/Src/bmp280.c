#include "bmp280.h"
#include "i2c.h"
#include <stdint.h>

uint16_t dig_T1;
int16_t dig_T2;
int16_t dig_T3;
int32_t t_fine;

void BMP280_Init(void) {
	// Read calibration values for compensation from NVM
	I2C_Start();
	I2C_WriteAddress(0x76, 0);
	I2C_WriteByte(0x88);
	I2C_Stop();
	I2C_Start();
	I2C_WriteAddress(0x76, 1);
	dig_T1 = I2C_ReadByte(1);
	dig_T1 |= (I2C_ReadByte(1) << 8);
	dig_T2 = I2C_ReadByte(1);
	dig_T2 |= (I2C_ReadByte(1) << 8);
	dig_T3 = I2C_ReadByte(1);
	dig_T3 |= (I2C_ReadByte(0) << 8);
	I2C_Stop();

	// Set Sensor to Forced Mode with over-sampling x1 for temperature and pressure
	I2C_Start();
	I2C_WriteAddress(0x76, 0);
	I2C_WriteByte(0xF4);
	I2C_WriteByte(0x25);
	I2C_Stop();
}

uint32_t BMP280_ReadTemp(void) {
	uint8_t measureStatus = 1;
	uint8_t msb;
	uint8_t lsb;
	uint8_t xlsb;
	uint32_t adcOut;

	// Set Sensor to Forced Mode to trigger measurement
	I2C_Start();
	I2C_WriteAddress(0x76, 0);
	I2C_WriteByte(0xF4);
	I2C_WriteByte(0x25);
	I2C_Stop();

	// Poll until status bit is set indicating measurements are in registers
	while((measureStatus & (1 << 3))!= 0) {
		I2C_Start();
		I2C_WriteAddress(0x76, 0);
		I2C_WriteByte(0xF3);
		I2C_Start();
		I2C_WriteAddress(0x76, 1);
		measureStatus = I2C_ReadByte(1);
		I2C_Stop();
	}

	// Read measurements
	I2C_Start();
	I2C_WriteAddress(0x76, 0);
	I2C_WriteByte(0xFA);
	I2C_Start();
	I2C_WriteAddress(0x76, 1);
	msb = I2C_ReadByte(1);
	lsb = I2C_ReadByte(1);
	xlsb = I2C_ReadByte(0);
	I2C_Stop();

	// Format and return measurement
	adcOut = (msb << 12) | (lsb << 4) | (xlsb >> 4);
	return adcOut;
}

int32_t BMP280_Compensate(int32_t adc_T) {
	int32_t var1, var2, T;
	var1  = ((((adc_T>>3) - ((int32_t)dig_T1<<1))) * ((int32_t)dig_T2)) >> 11;
	var2  = (((((adc_T>>4) - ((int32_t)dig_T1)) * ((adc_T>>4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
	t_fine = var1 + var2;
	T  = (t_fine * 5 + 128) >> 8;
	return T;
}
