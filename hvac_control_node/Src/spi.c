#include "spi.h"
#include <stdint.h>

#define RCC_APB2ENR   (*(volatile uint32_t*)0x40021018)
#define GPIOA_CRL     (*(volatile uint32_t*)0x40010800)
#define GPIOA_ODR     (*(volatile uint32_t*)0x4001080C)
#define SPI1_CR1      (*(volatile uint32_t*)0x40013000)
#define SPI1_SR       (*(volatile uint32_t*)0x40013008)
#define SPI1_DR       (*(volatile uint32_t*)0x4001300C)

void SPI_Init(void) {
	// Enable clocks
	RCC_APB2ENR |= (1 << 2); // Enable clock for GPIOA
	RCC_APB2ENR |= (1 << 12); // Enable clock for SPI1

	// Set GPIOA pins to output/input
	SPI_CS_High(); // Drive pin 4 high immediately to eliminate current spike
	GPIOA_CRL &= ~(0xF << 16); // Clear pin 4
	GPIOA_CRL |= (0x3 << 16); // Set pin 4 to general purpose push pull output
	GPIOA_CRL &= ~(0xF << 20); // Clear pin 5
	GPIOA_CRL |= (0xB << 20); // Set pin 5 to alternate function push pull output
	GPIOA_CRL &= ~(0xF << 24); // Clear pin 6
	GPIOA_CRL |= (0x4 << 24); // Set pin 6 to floating input
	GPIOA_CRL &= ~(0xF << 28); // Clear pin 7
	GPIOA_CRL |= (0xB << 28); // Set pin 7 to alternate function push pull output

	// Setup SPI peripheral
	SPI1_CR1 |= (1 << 2); // Set SPI to master mode
	SPI1_CR1 &= ~(0x7 << 3); // Clear bits 5:3 for baud (clock) rate
	SPI1_CR1 |= (0x7 << 3); // Set baud rate to 281 kHz
	SPI1_CR1 |= (0x3 << 8); // Ignore NSS pin
	SPI1_CR1 |= (1 << 6); // Set bit 6 to enable SPI
}

uint8_t SPI_Transfer(uint8_t data) {
	for(int i = 0; i < 100000; i++) if(SPI1_SR & (1 << 1)) break; // Waits until status bit is set by hardware
	SPI1_DR = (data); // Writes data to SPI data register for MOSI
	for(int i = 0; i < 100000; i++) if(SPI1_SR & (1)) break; // Waits until status bit is set by hardware
	uint8_t input = SPI1_DR; // Reads byte from MISO
	for(int i = 0; i < 100000; i++) if(!(SPI1_SR & (1 << 7))) break; // Waits until status bit is cleared by hardware
	return input; // Returns byte
}

void SPI_CS_Low(void) {
	GPIOA_ODR &= ~(1 << 4); // Clear pin 4 of ODR to set CS low
}

void SPI_CS_High(void) {
	GPIOA_ODR |= (1 << 4); // Set pin 4 of ODR to set CS high
}
