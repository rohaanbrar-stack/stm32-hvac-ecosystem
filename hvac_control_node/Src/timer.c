#include "timer.h"
#include <stdint.h>

#define RCC_APB1ENR   (*(volatile uint32_t*)0x4002101C)
#define NVIC_ISER0    (*(volatile uint32_t*)0xE000E100)
#define TIM2_CR1      (*(volatile uint32_t*)0x40000000)
#define TIM2_DIER     (*(volatile uint32_t*)0x4000000C)
#define TIM2_PSC      (*(volatile uint32_t*)0x40000028)
#define TIM2_ARR      (*(volatile uint32_t*)0x4000002C)
#define TIM2_SR       (*(volatile uint32_t*)0x40000010)

volatile uint8_t sample_flag;
volatile uint32_t timestamp;

void TIM2_Init(void) {
	RCC_APB1ENR |= (1); // Enable TIM2 clock
	TIM2_PSC &= ~(0xFFFF); // Clear prescaler
	TIM2_PSC |= (0x57E3); // Set prescaler to 22499
	TIM2_ARR &= ~(0xFFFF); // Clear auto reload
	TIM2_ARR |= (0xF); // Set auto reload to 15 (sets clock rate to 100 Hz )
	TIM2_DIER |= (0x1); // Enable update interrupt
	NVIC_ISER0 |= (0x1 << 28); // Enable IRQ28 (internal interrupt)
	TIM2_CR1 |= (0x1); // Enable counter
}

void TIM2_IRQHandler(void) {
	TIM2_SR &= ~(0x1); // Clear update interrupt flag
	sample_flag = 1; // Set timer variable
	timestamp++; // 10 ms per IRQ
}
