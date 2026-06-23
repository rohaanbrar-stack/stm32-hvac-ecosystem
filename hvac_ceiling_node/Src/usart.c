#include "usart.h"
#include <stdint.h>

#define RCC_APB2ENR   (*(volatile uint32_t*)0x40021018)
#define GPIOA_CRH     (*(volatile uint32_t*)0x40010804)
#define GPIOA_ODR     (*(volatile uint32_t*)0x4001080C)
#define USART1_SR    (*(volatile uint32_t*)0x40013800)
#define USART1_DR    (*(volatile uint32_t*)0x40013804)
#define USART1_BRR    (*(volatile uint32_t*)0x40013808)
#define USART1_CR1    (*(volatile uint32_t*)0x4001380C)

void USART_Init(void) {
	RCC_APB2ENR |= (0x1 << 2); // Enable GPIOA
	RCC_APB2ENR |= (0x1 << 14); // Enable USART1
	GPIOA_CRH &= ~(0xFF << 4); // Clear bits 11:4
	GPIOA_CRH |= (0x9 << 4); // Set pin 9 alternate function push pull 10Mhz
	GPIOA_CRH |= (0x8 << 8); // Set pin 10 pull up/pull down input
	GPIOA_ODR |= (0x1 << 10); // Set pull up for pin 10
	USART1_BRR &= ~(0xFFFF); // Clear bits 15:0
	USART1_BRR |= (0x139); // Set BRR to 0x139 (Calculated from USARTDIV)
	USART1_CR1 |= (0x200C); // Enable USART, TX, RX
}

void USART_WriteByte(uint8_t byte) {
	USART1_DR = (byte); // Write byte
	while(!(USART1_SR & (0x1 << 7))); // Poll until status register confirms byte sent
}

