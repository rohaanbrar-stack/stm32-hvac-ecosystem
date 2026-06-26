#include "clock.h"
#include "usart.h"
#include "spi.h"
#include "nRF24.h"
#include <stdint.h>
#include <stdio.h>

int main(void)
{
	// Driver initializations
	Clock_Init();
	USART_Init();
    SPI_Init();
    nRF24_Init();

    // Variable declarations
    uint8_t nRF_Val;
    char buffer[40];

    // nRF24 test
    for(int i = 0; i < 1000000; i++);
    nRF_Val = nRF24_ReadReg(0x00);
    sprintf(buffer, "%02X\r\n", nRF_Val);
    int i = 0;
    while(buffer[i] != '\0') {
    	USART_WriteByte(buffer[i]);
    	i++;
    }
}
