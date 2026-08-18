#ifndef USART_H
#define USART_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    volatile uint32_t SR;    // 4 bytes → offset 0
    volatile uint32_t DR;    // 4 bytes → offset 4
    volatile uint32_t BRR;   // 4 bytes → offset 8
    volatile uint32_t CR1;   // 4 bytes → offset 12 (0x0C)
} USART_t;

#define USART1 ((USART_t*)0x40013800)
#define USART2 ((USART_t*)0x40004400)

void USART1_Init(void);
void USART2_Init(void);
void USART_WriteByte(USART_t*, uint8_t);
bool USART_ReadByte(USART_t*, uint8_t*, uint32_t);
bool USART_DataAvailable(USART_t *u);

#endif
