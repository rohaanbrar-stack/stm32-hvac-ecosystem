#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void TIM2_Init(void);
void TIM2_IRQHandler(void);
extern volatile uint8_t sample_flag;
extern volatile uint32_t timestamp;

#endif
