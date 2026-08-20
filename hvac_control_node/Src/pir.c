#include "pir.h"
#include "timer.h"
#include <stdint.h>

#define RCC_APB2ENR   (*(volatile uint32_t*)0x40021018)
#define GPIOB_CRL     (*(volatile uint32_t*)0x40010C00)
#define AFIO_EXTICR1  (*(volatile uint32_t*)0x40010008)
#define EXTI_RTSR     (*(volatile uint32_t*)0x40010408)
#define EXTI_IMR      (*(volatile uint32_t*)0x40010400)
#define EXTI_PR       (*(volatile uint32_t*)0x40010414)
#define NVIC_ISER0    (*(volatile uint32_t*)0xE000E100)

volatile uint32_t last_motion;

void PIR_Init(void) {
	RCC_APB2ENR |= (0x1 << 3); // Enable GPIOB
	RCC_APB2ENR |= (0x1); // Enable AFIOEN
	GPIOB_CRL &= ~(0xF); // Clear bits 3:0
	GPIOB_CRL |= (0x4); // Set pin 0 floating input
	AFIO_EXTICR1 &= ~(0xF); // Clear bits 3:0
	AFIO_EXTICR1 |= (0x1); // Route port B to EXTI0
	EXTI_RTSR |= (0x1); // Set rising edge
	EXTI_IMR |= (0x1); // Unmask
	NVIC_ISER0 |= (0x1 << 6); // Enable IRQ6 (internal interrupt)
}

void EXTI0_IRQHandler(void) {
	last_motion = timestamp; // Set last motion to current timestamp
	EXTI_PR = (1 << 0); // Clear flag
}
