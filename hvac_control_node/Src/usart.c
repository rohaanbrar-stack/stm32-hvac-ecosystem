#include "usart.h"
#include <stdint.h>
#include <stdbool.h>

#define RCC_APB2ENR   (*(volatile uint32_t*)0x40021018)
#define RCC_APB1ENR   (*(volatile uint32_t*)0x4002101C)
#define GPIOA_CRL     (*(volatile uint32_t*)0x40010800)
#define GPIOA_CRH     (*(volatile uint32_t*)0x40010804)
#define GPIOA_ODR     (*(volatile uint32_t*)0x4001080C)
#define USART1_SR     (*(volatile uint32_t*)0x40013800)
#define USART1_DR     (*(volatile uint32_t*)0x40013804)
#define USART1_BRR    (*(volatile uint32_t*)0x40013808)
#define USART1_CR1    (*(volatile uint32_t*)0x4001380C)
#define USART2_SR     (*(volatile uint32_t*)0x40004400)
#define USART2_DR     (*(volatile uint32_t*)0x40004404)
#define USART2_BRR    (*(volatile uint32_t*)0x40004408)
#define USART2_CR1    (*(volatile uint32_t*)0x4000440C)

void USART1_Init(void) {
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

void USART2_Init(void) {
	RCC_APB2ENR |= (0x1 << 2); // Enable GPIOA
	RCC_APB1ENR |= (0x1 << 17); // Enable USART2
	GPIOA_CRL &= ~(0xFF << 8); // Clear bits 15:8
	GPIOA_CRL |= (0xB << 8); // Set pin 2 alternate function push pull 50Mhz
	GPIOA_CRL |= (0x8 << 12); // Set pin 3 pull up/pull down input
	GPIOA_ODR |= (0x1 << 3); // Set pull up for pin 3
	USART2_BRR &= ~(0xFFFF); // Clear bits 15:0
	USART2_BRR |= (0xEA6); // Set BRR to 0xEA6 (Calculated from USARTDIV)
	USART2_CR1 |= (0x200C); // Enable USART, TX, RX
}

void USART_WriteByte(USART_t *u, uint8_t byte) {
	u->DR = (byte); // Write byte
	while(!(u->SR & (0x1 << 7))); // Poll until status register confirms byte sent
}

uint8_t USART_ReadByte(USART_t *u) {
	while(!(u->SR & (0x1 << 5))); // Poll until status register confirms byte received
	return u->DR; // Read byte
}
