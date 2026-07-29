#include "i2c.h"
#include <stdint.h>

#define RCC_APB2ENR   (*(volatile uint32_t*)0x40021018)
#define RCC_APB1ENR   (*(volatile uint32_t*)0x4002101C)
#define GPIOB_CRL     (*(volatile uint32_t*)0x40010C00)
#define I2C_CR1       (*(volatile uint32_t*)0x40005400)
#define I2C_CR2       (*(volatile uint32_t*)0x40005404)
#define I2C_CCR       (*(volatile uint32_t*)0x4000541C)
#define I2C_TRISE     (*(volatile uint32_t*)0x40005420)
#define I2C_DR        (*(volatile uint32_t*)0x40005410)
#define I2C_SR1       (*(volatile uint32_t*)0x40005414)
#define I2C_SR2       (*(volatile uint32_t*)0x40005418)

void I2C_Init(void) {
	// Enable clocks
	RCC_APB2ENR |= (1 << 3); // Enable clock for GPIOB
	RCC_APB1ENR |= (1 << 21); // Enable clock for I2C1

	// Set GPIOB pins to output
	GPIOB_CRL &= ~(0xF << 24); // Clear pin 6
	GPIOB_CRL |= (0xE << 24); // Set pin 6 to alternate function open drain
	GPIOB_CRL &= ~(0xF << 28); // Clear pin 7
	GPIOB_CRL |= (0xE << 28); // Set pin 7 to alternate function open drain

	// Setup I2C peripheral clock
	I2C_CR1 &= ~(0x1); // Clear bit 0 to disable peripheral (required for CCR)
	I2C_CR2 &= ~(0x3F); // Clear bits 5:0 for frequency
	I2C_CR2 |= (0x24); // Set frequency to 36 MHz
	I2C_CCR &= ~(0xFFF); // Clear bits 11:0 for CCR
	I2C_CCR |= (0x1D); // Set clock to 400 kHZ SCL frequency
	I2C_CCR |= (0x1 << 15); // Set clock to fast mode
	I2C_TRISE &= ~(0x1F); // Clear bits 5:0 for TRISE
	I2C_TRISE |= (0xB); // Set fast mode
	I2C_CR1 |= (0x1); // Set bit 0 to enable peripheral
}

void I2C_Start(void) {
	I2C_CR1 |= (0x1 << 8); // Set bit 8 for start condition
	while(!(I2C_SR1 & (1))); // Waits until status bit is set by hardware
}

void I2C_Stop(void) {
	I2C_CR1 |= (0x1 << 9); // Set bit 9 for stop condition
}

void I2C_WriteAddress(uint8_t addr, uint8_t rw) {
	I2C_DR = (addr << 1) | rw; // Construct and set bits 7:0 for address and direction (0 is write, 1 is read)
	while(!(I2C_SR1 & (1 << 1))); // Waits until status bit is set by hardware
	(void)I2C_SR1; // Read SR1
	(void)I2C_SR2; // Read SR2 to clear ADDR
}

void I2C_WriteByte(uint8_t byte) {
	I2C_DR = (byte); // Set bits 7:0 for byte to be transmitted
	while(!(I2C_SR1 & (1 << 7))); // Waits until status bit is set by hardware
}

uint8_t I2C_ReadByte(uint8_t ack) {
	I2C_CR1 &= ~(0x1 << 10); // Clear bit 10 for acknowledge
	I2C_CR1 |= (ack << 10); // Set bit 10 to ACK or NACK (0 is NACK, 1 is ACK)
	while(!(I2C_SR1 & (1 << 6))); // Waits until status bit is set by hardware
	uint8_t byte = I2C_DR; // Read received byte
	return byte; // Return byte
}


