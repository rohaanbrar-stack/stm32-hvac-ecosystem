#ifndef BMP280_H
#define BMP280_H

#include <stdint.h>

void BMP280_Init(void);
uint32_t BMP280_ReadTemp(void);
int32_t BMP280_Compensate(int32_t);

#endif
