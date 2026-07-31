#include "timer.h"
#include <stdint.h>

#define RCC_APB1ENR   (*(volatile uint32_t*)0x4002101C)
#define NVIC_ISER0    (*(volatile uint32_t*)0xE000E100)
#define TIM3_CR1      (*(volatile uint32_t*)0x40000400)
#define TIM3_DIER     (*(volatile uint32_t*)0x4000040C)
#define TIM3_PSC      (*(volatile uint32_t*)0x40000428)
#define TIM3_ARR      (*(volatile uint32_t*)0x4000042C)
#define TIM3_SR       (*(volatile uint32_t*)0x40000410)

volatile uint8_t sample_flag;
volatile uint32_t timestamp;

void TIM3_Init(void) {
	RCC_APB1ENR |= (1 << 1); // Enable TIM3 clock
	TIM3_PSC &= ~(0xFFFF); // Clear prescaler
	TIM3_PSC |= (0x57E3); // Set prescaler to 22499
	TIM3_ARR &= ~(0xFFFF); // Clear auto reload
	TIM3_ARR |= (0xF); // Set auto reload to 15 (sets clock rate to 100 Hz )
	TIM3_DIER |= (0x1); // Enable update interrupt
	NVIC_ISER0 |= (0x1 << 29); // Enable IRQ29 (internal interrupt)
	TIM3_CR1 |= (0x1); // Enable counter
}

void TIM3_IRQHandler(void) {
	TIM3_SR &= ~(0x1); // Clear update interrupt flag
	sample_flag = 1; // Set timer variable
	timestamp++; // 10 ms per IRQ
}
