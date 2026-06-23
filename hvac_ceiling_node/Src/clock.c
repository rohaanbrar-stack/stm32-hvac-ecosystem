#include "clock.h"
#include <stdint.h>

#define FLASH_ACR   (*(volatile uint32_t*)0x40022000)
#define RCC_CR      (*(volatile uint32_t*)0x40021000)
#define RCC_CFGR    (*(volatile uint32_t*)0x40021004)
#define GPIOC_ODR     (*(volatile uint32_t*)0x4001100C)

void Clock_Init(void) {
	FLASH_ACR &= ~(0x7); // Clear bits 2:0
	FLASH_ACR |= (0x1); // Set Flash latency to 1 state
	FLASH_ACR |= (0x1 << 4); // Enable prefetch
	RCC_CFGR &= ~(0xF << 18); // Clear bits 21:18
	RCC_CFGR |= (0x7 << 18); // Set PLLMUL to 9x
	RCC_CFGR &= ~(0x7 << 8); // Clear bits 10:8
	RCC_CR |= (0x1 << 24); // Enable PLL
	while(!(RCC_CR & (0x1 << 25))); // Wait for PLL to be ready
	RCC_CFGR &= ~(0x3); // Clear bits 1:0
	RCC_CFGR |= (0x1 << 1); // Set PLL as clock system
	while((RCC_CFGR & (0x3 << 2)) != (0x2 << 2)); // Wait to confirm switch
}
