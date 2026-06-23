#include "servo.h"
#include <stdint.h>

#define RCC_APB1ENR   (*(volatile uint32_t*)0x4002101C)
#define GPIOA_CRL     (*(volatile uint32_t*)0x40010800)
#define TIM2_CR1      (*(volatile uint32_t*)0x40000000)
#define TIM2_CCMR1    (*(volatile uint32_t*)0x40000018)
#define TIM2_CCER     (*(volatile uint32_t*)0x40000020)
#define TIM2_PSC      (*(volatile uint32_t*)0x40000028)
#define TIM2_ARR      (*(volatile uint32_t*)0x4000002C)
#define TIM2_CCR1     (*(volatile uint32_t*)0x40000034)
#define TIM2_CCR2     (*(volatile uint32_t*)0x40000038)

void Servo_Init(void) {
	RCC_APB1ENR |= (1); // Enable TIM2 clock
	GPIOA_CRL &= ~(0xFF); // Clear configurations for PA0 and PA1
	GPIOA_CRL |= (0xBB); // Set PA0 and PA1 to alternate function push pull outputs at 50Mhz
	TIM2_PSC &= ~(0xFFFF); // Clear prescaler
	TIM2_PSC |= (0xB); // Set prescaler to 11
	TIM2_ARR &= ~(0xFFFF); // Clear auto reload
	TIM2_ARR |= (0xEA5F); // Set auto reload to 59999 (sets clock rate to 50Hz)
	TIM2_CCMR1 &= ~(0x7373); // Clear CC1S (1:0), CC2S (9:8), OC1M (6:4), OC2M (14:12)
	TIM2_CCMR1 |= (0x6060); // Configure CH1 and CH2 to output PWM mode 1
	TIM2_CCR1 = (0x1194); // Set CCR1 to neutral 90 degrees
	TIM2_CCR2 = (0x1194); // Set CCR2 to neutral 90 degrees
	TIM2_CCER |= (0x11); // Enable CH1 and CH2 outputs
	TIM2_CR1 |= (0x1); // Enable counter
}

void Servo_SetAngle(uint8_t angle, uint8_t servo) {
	switch(servo) {
		case 1:
			TIM2_CCR1 = ((angle * 100) / 3) + 1500;
			break;
		case 2:
			TIM2_CCR2 = ((angle * 100) / 3) + 1500;
			break;
	}
}
