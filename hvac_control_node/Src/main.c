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
    char buffer[40];
    uint8_t bytes[10];

    // nRF24 test
    for(int i = 0; i < 1000000; i++);
    sprintf(buffer, "Here\r\n");
    int i = 0;
    while(buffer[i] != '\0') {
        USART_WriteByte(buffer[i]);
        i++;
    }
    nRF24_CE_High();
    while(1) {
        uint8_t status = nRF24_ReadReg(0x07);
        sprintf(buffer, "ST: %02X\r\n", status);
        int k = 0;
        while(buffer[k] != '\0') {
            USART_WriteByte(buffer[k]);
            k++;
        }
        if(status & (0x01 << 6)) break; // RX_DR set, exit loop
        for(int d = 0; d < 2000000; d++); // ~delay
    }
    nRF24_ReadPayload(bytes, 4);
    for(int j = 0; j < 4; j++) {
    	sprintf(buffer, "%02X\r\n", bytes[j]);
    	i = 0;
    	while(buffer[i] != '\0') {
    		USART_WriteByte(buffer[i]);
    		i++;
    	}
    }
}
