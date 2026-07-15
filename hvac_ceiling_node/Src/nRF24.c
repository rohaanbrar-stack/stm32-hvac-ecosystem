#include "nRF24.h"
#include "spi.h"
#include "usart.h"
#include <stdint.h>
#include <stdio.h>

#define RCC_APB2ENR   (*(volatile uint32_t*)0x40021018)
#define RCC_CSR  	  (*(volatile uint32_t *)0x40021024)
#define GPIOB_CRL     (*(volatile uint32_t*)0x40010C00)
#define GPIOB_ODR     (*(volatile uint32_t*)0x40010C0C)

static void print_reg(char *label, uint8_t val) {
	char buffer[40];
	sprintf(buffer, "%s: %02X\r\n", label, val);
	int i = 0;
	while(buffer[i] != '\0') { USART_WriteByte(buffer[i]); i++; }
}

static void print_str(char *s) {
    int i = 0;
    while (s[i] != '\0') { USART_WriteByte(s[i]); i++; }
}

void nRF24_WriteRegVerified(uint8_t addr, uint8_t val) {
    char buf[40];
    for (int attempt = 0; attempt < 8; attempt++) {
        nRF24_WriteReg(addr, val);
        nRF24_ReadReg(addr);                 // throwaway — absorbs the desync
        uint8_t rb = nRF24_ReadReg(addr);    // this one is clean
        if (rb == val) {
            sprintf(buf, "W %02X=%02X ok(%d)\r\n", addr, val, attempt);
            print_str(buf);
            return;
        }
    }
    sprintf(buf, "W %02X=%02X FAIL\r\n", addr, val);
    print_str(buf);
}

void print_reset_cause(void) {                 // <-- snippet 3, defined here
    uint32_t r = RCC_CSR;
    char buffer[48];
    sprintf(buffer, "RST:%s%s%s%s%s\r\n",
        (r & (1u<<27)) ? " POR"  : "",
        (r & (1u<<29)) ? " IWDG" : "",
        (r & (1u<<30)) ? " WWDG" : "",
        (r & (1u<<28)) ? " SFT"  : "",
        (r & (1u<<26)) ? " PIN"  : "");
    print_str(buffer);
    RCC_CSR |= (1u<<24);
}

void nRF24_Init(void) {
	uint8_t address[] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
	volatile uint32_t VAR = 55000;

	RCC_APB2ENR |= (1 << 3); // Enable clock for GPIOB
	GPIOB_CRL &= ~(0xF); // Clear pin 0
	GPIOB_CRL |= (0x3); // Set pin 0 to general purpose push pull output
	nRF24_CE_Low(); // Set CE low to start
	nRF24_WriteRegVerified(0x00, 0x08 | 0x02); // Power and set nRF to TX mode (OR with 0x08 to not disrupt EN_CRC)

	for(int i = 0; i < VAR; i++); // Wait ~1.5ms for power up
	nRF24_WriteRegVerified(0x05, 0x4C); // Sets radio to channel 76

	nRF24_WriteRegVerified(0x06, 0x26);

	nRF24_WriteRegVerified(0x04, 0x53);

	nRF24_WriteRegMulti(0x10, address, 5); // Sets 5 byte address for TX_ADDR

	nRF24_WriteRegMulti(0x0A, address, 5); // Sets 5 byte address for RX_ADDR_P0

}

void nRF24_Dump(void) {
    uint8_t a[5];
    char buffer[48];
    nRF24_ReadReg(0x07);            // sacrificial resync after init's writes

    print_reg("CFG",  nRF24_ReadReg(0x00));
    print_reg("AA",   nRF24_ReadReg(0x01));
    print_reg("RXEN", nRF24_ReadReg(0x02));
    print_reg("AW",   nRF24_ReadReg(0x03));
    print_reg("RETR", nRF24_ReadReg(0x04));
    print_reg("CH",   nRF24_ReadReg(0x05));
    print_reg("SET",  nRF24_ReadReg(0x06));

    nRF24_ReadRegMulti(0x0A, a, 5);
    sprintf(buffer, "RXP0: %02X %02X %02X %02X %02X\r\n", a[0],a[1],a[2],a[3],a[4]);
    print_str(buffer);

    nRF24_ReadRegMulti(0x10, a, 5);
    sprintf(buffer, "TXAD: %02X %02X %02X %02X %02X\r\n", a[0],a[1],a[2],a[3],a[4]);
    print_str(buffer);

    print_reg("PW0",  nRF24_ReadReg(0x11));
    print_reg("FIFO", nRF24_ReadReg(0x17));
}

void nRF24_WriteReg(uint8_t addr, uint8_t byte) {
	// Write sequence
	SPI_CS_Low();
	SPI_Transfer(addr | 0x20);
	SPI_Transfer(byte);
	SPI_CS_High();
	nRF24_ReadReg(0x07);
}

void nRF24_WriteRegMulti(uint8_t addr, uint8_t *bytes, uint8_t len) {
	// Write sequence
	SPI_CS_Low();
	SPI_Transfer(addr | 0x20);
	for(int i = 0; i < len; i++) {
		SPI_Transfer(bytes[i]); // Write len number of bytes through burst write
	}
	SPI_CS_High();
	nRF24_ReadReg(0x07);
}

uint8_t nRF24_ReadReg(uint8_t addr) {
	uint8_t read; // Read value

	// Read sequence
	SPI_CS_Low();
	SPI_Transfer(addr);
	read = SPI_Transfer(0xFF);
	SPI_CS_High();

	return read; // Return read value
}

void nRF24_ReadRegMulti(uint8_t reg, uint8_t *buf, uint8_t len) {
	SPI_CS_Low();
    SPI_Transfer(reg & 0x1F);           // R_REGISTER = 0x00, so just the addr
    for (uint8_t i = 0; i < len; i++)
        buf[i] = SPI_Transfer(0xFF);    // clock out dummy, read MISO
    SPI_CS_High();
}

void nRF24_WritePayload(uint8_t *bytes, uint8_t len) {
	// Write sequence
	SPI_CS_Low();
	SPI_Transfer(0xA0);
	for(int i = 0; i < len; i++) {
		SPI_Transfer(bytes[i]); // Write len number of bytes through burst write
	}
	SPI_CS_High();
	nRF24_ReadReg(0x07);

	// Pulse transmission
	nRF24_CE_High(); // Pull CE high to send transmission
	for(volatile uint32_t i = 0; i < 360; i++); // Wait 10us
	nRF24_CE_Low(); // Pull CE low
}

void nRF24_FlushTX(void) {
    SPI_CS_Low();
    SPI_Transfer(0xE1);
    SPI_CS_High();
    nRF24_ReadReg(0x07);
}

void nRF24_CE_Low(void) {
	GPIOB_ODR &= ~(1); // Clear pin 0 of ODR to set CE low
}

void nRF24_CE_High(void) {
	GPIOB_ODR |= (1); // Set pin 0 of ODR to set CE high
}
