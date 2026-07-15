#include "clock.h"
#include "usart.h"
#include "spi.h"
#include "nRF24.h"
#include <stdint.h>
#include <stdio.h>

#define GPIOB_ODR     (*(volatile uint32_t*)0x40010C0C)

int main(void)
{
	volatile uint32_t MAXM = 1000000;

	// Driver initializations
	Clock_Init();
	USART_Init();
	print_reset_cause();
    SPI_Init();
    for(int i = 0; i < MAXM; i++);
    nRF24_Init();
    nRF24_Dump();

    // Variable declarations
    char buffer[48];
    uint8_t bytes[10];

    // nRF24 test
    for(int i = 0; i < MAXM; i++);
    sprintf(buffer, "Here6\r\n");
    int i = 0;
    while(buffer[i] != '\0') {
        USART_WriteByte(buffer[i]);
        i++;
    }

    uint8_t cfg = nRF24_ReadReg(0x00);
    sprintf(buffer, "CFG: %02X\r\n", cfg);
    i = 0;
    while(buffer[i] != '\0') { USART_WriteByte(buffer[i]); i++; }

    uint8_t ch = nRF24_ReadReg(0x05);
    sprintf(buffer, "CH: %02X\r\n", ch);
    i = 0;
    while(buffer[i] != '\0') { USART_WriteByte(buffer[i]); i++; }

    uint8_t set = nRF24_ReadReg(0x06);
    sprintf(buffer, "SET: %02X\r\n", set);
    i = 0;
    while(buffer[i] != '\0') { USART_WriteByte(buffer[i]); i++; }

    uint8_t pw = nRF24_ReadReg(0x11);
    sprintf(buffer, "PW: %02X\r\n", pw);
    i = 0;
    while(buffer[i] != '\0') { USART_WriteByte(buffer[i]); i++; }

    nRF24_CE_High();
    uint8_t ce = (GPIOB_ODR >> 0) & 1;
    sprintf(buffer, "CE: %02X\r\n", ce);
    i = 0;
    while(buffer[i] != '\0') { USART_WriteByte(buffer[i]); i++; }

    while(1) {
    	uint8_t status = nRF24_ReadReg(0x07);
    	uint8_t rpd    = nRF24_ReadReg(0x09) & 0x01;   // 1 = signal in the air right now
    	uint8_t fifo   = nRF24_ReadReg(0x17);
    	sprintf(buffer, "ST:%02X RPD:%d FIFO:%02X\r\n", status, rpd, fifo);
        int k = 0;
        while(buffer[k] != '\0') {
            USART_WriteByte(buffer[k]);
            k++;
        }
        if(status & (0x01 << 6)) {
        	while((nRF24_ReadReg(0x17) & 0x01) == 0) {
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
        	nRF24_WriteReg(0x07, (0x01 << 6));
        }
        for(int d = 0; d < 2000000; d++); // ~delay
    }
}
