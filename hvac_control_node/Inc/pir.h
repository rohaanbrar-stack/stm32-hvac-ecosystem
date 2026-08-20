#ifndef PIR_H
#define PIR_H

#include <stdint.h>

void PIR_Init(void);
void EXTI0_IRQHandler(void);
extern volatile uint32_t last_motion;

#endif
