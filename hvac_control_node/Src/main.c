#include "clock.h"
#include "bmp280.h"
#include "i2c.h"
#include "usart.h"
#include "spi.h"
#include "timer.h"
#include "ssd1306.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define SET 2556
#define D 85
#define M 110
#define FRAME_START 150
#define FRAME_BYTE 5

typedef enum {CLOSED, COOLING, HEATING} ventState;
static const char *stateNames[] = {"CLOSED", "COOLING", "HEATING"};

void send_frame(uint8_t type, uint8_t *payload, uint8_t len) {
	USART_WriteByte(USART2, 0xAA); // Send start byte
	uint8_t checksum = type; // Add command type to checksum
	USART_WriteByte(USART2, type); // Send command type
	for(int i = 0; i < len; i++) { USART_WriteByte(USART2, payload[i]); checksum += payload[i]; } // Send each payload byte and add to checksum
	USART_WriteByte(USART2, checksum); // Send checksum
}

bool receive_frame(uint8_t *type, uint8_t *payload) {
	uint8_t byte;
	do {
		if(!USART_ReadByte(USART2, &byte, FRAME_START)) return false;
	} while(byte != 0xAA); // Poll until start byte is received
	if(!USART_ReadByte(USART2, type, FRAME_BYTE)) return false; // Read type byte
	uint8_t len;
	switch(*type) {    // Check type byte for payload size
		case 0x01:
			len = 2;
			break;
		case 0x02:
			len = 1;
			break;
		case 0x03:
			len = 1;
			break;
		default:
			return false;
	}
	uint8_t checksum = *type;
	for(int i = 0; i < len; i++) {if(!USART_ReadByte(USART2, &payload[i], FRAME_BYTE)) return false; checksum += payload[i]; } // Read each payload byte and add to checksum
	uint8_t checkTX;
	if(!USART_ReadByte(USART2, &checkTX, FRAME_BYTE)) return false; // Read checksum byte
	if(checksum == checkTX) { return true; } // Compare checksums for corruption
	else return false;
}

int main(void)
{
	char buffer[48];

	// Driver initializations
	Clock_Init();
	TIM2_Init();
	USART1_Init();
	USART2_Init();
	USART3_Init();
	I2C_Init();
	BMP280_Init();
    SPI_Init();
    SSD1306_Init();

    // Variable declarations
    uint8_t type;
    uint8_t payload[2];
    uint32_t adc_T;
    int32_t temp;
    int32_t tempF;
    int32_t tempVent;
    int32_t tempVentF;
    int32_t setF;
    uint8_t cmd;
    ventState state = CLOSED;

    // OLED Refresh
    SSD1306_Clear();
    SSD1306_Refresh();

    while(1) {

    	// Take temperature measurement
    	adc_T = BMP280_ReadTemp();
    	temp = BMP280_Compensate(adc_T);

    	// Read temperature from ceiling node
    	bool conf = receive_frame(&type, payload);

    	// Check for command type
    	if(conf && type == 0x01) { // Temperature
    		tempVent = (int16_t)((payload[0] << 8) | payload[1]);

    		// Send servo controls
    		switch(state) {    // Check current vent state
    			case CLOSED:
    				if((temp > (SET + D)) && (tempVent < (temp - M))) {cmd = 180; send_frame(0x02, &cmd, 1); state = COOLING; break;} // Open duct for cooling
    				else if((temp < (SET - D)) && (tempVent > (temp + M))) {cmd = 180; send_frame(0x02, &cmd, 1); state = HEATING; break;} // Open duct for heating
    				else break;
    			case COOLING:
    				if(temp <= SET) {cmd = 0; send_frame(0x02, &cmd, 1); state = CLOSED; break;} // Close duct if room temperature is right
    				else if(tempVent >= temp) {cmd = 0; send_frame(0x02, &cmd, 1); state = CLOSED; break;} // Close duct if duct is hot when it should be cold
    				else break;
    			case HEATING:
    				if(temp >= SET) {cmd = 0; send_frame(0x02, &cmd, 1); state = CLOSED; break;} // Close duct if room temperature is right
    				else if(tempVent <= temp) {cmd = 0; send_frame(0x02, &cmd, 1); state = CLOSED; break;} // Close duct if duct is cold when it should be hot
    				else break;
    			default:
    				break;
    		}
    		bool wontHelp = (state == CLOSED) && ((temp > SET + D) || (temp < SET - D)); // Set WONTHELP boolean for display

    		// Print temperature to USART1
    		sprintf(buffer, "%ld, %d, %ld, %d, %d, %d\r\n", temp, type, tempVent, conf, state, wontHelp);
    		int i = 0;
    		while(buffer[i] != '\0') { USART_WriteByte(USART1, buffer[i]); i++; }

    		// Celsius to Fahrenheit conversions
    		tempVentF = (tempVent * 9) / 50 + 320;
    		tempF = (temp * 9) / 50 + 320;
    		setF = (SET * 9) / 50 + 320;

    		// OLED Display
    		SSD1306_Clear();
    		sprintf(buffer, "ROOM: %ld.%ld F", tempF / 10, tempF % 10);
    		SSD1306_DrawString(buffer, 2, 2);
    		sprintf(buffer, "DUCT: %ld.%ld F", tempVentF / 10, tempVentF % 10);
    		SSD1306_DrawString(buffer, 2, 11);
    		sprintf(buffer, "SET: %ld.%ld F", setF / 10, setF % 10);
    		SSD1306_DrawString(buffer, 2, 20);
    		sprintf(buffer, "VENT %s", stateNames[state]);
    		SSD1306_DrawString(buffer, 2, 29);
    		if(wontHelp) {
    			SSD1306_DrawString("VENT WON'T HELP", 2, 38);
    		}
    		SSD1306_Refresh();

    		// Print temperature to ESP8266
    		sprintf(buffer, "T %ld, %ld, %d, %d\n", temp, tempVent, state, SET);
    		for(int i = 0; buffer[i] != '\0'; i++) {
    		    USART_WriteByte(USART3, buffer[i]);
    		}
    	}
    	else if(conf && type == 0x03) { // ACK
    		sprintf(buffer, "ACK\r\n");
    		int i = 0;
    		while(buffer[i] != '\0') { USART_WriteByte(USART1, buffer[i]); i++; }
    	}
    	else {
    		sprintf(buffer, "NOFRAME, %ld\r\n", timestamp);
    		int i = 0;
    		while(buffer[i] != '\0') { USART_WriteByte(USART1, buffer[i]); i++; }
    	}
    }
}
