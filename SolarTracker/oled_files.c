/*
 * oledfiles.c
 *
 * Created: 06/10/2022 21:00:09
 *  Author: Peter
 */ 
 #include <avr/eeprom.h>
 #include <string.h>
 #include <inttypes.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <util/delay.h>
 #include <avr/pgmspace.h>
 #include <avr/io.h>
 #include <avr/interrupt.h>
 #include <avr/sfr_defs.h>
 #include <math.h>
 #include "nmea.h"
 #include "i2cmaster.h"
 #include "defines.h"
 #include <time.h>
// #include <util/eu_dst.h>
 #include <inttypes.h>
 #include <stdbool.h>
 #include "oled_files.h"

 static const unsigned char PROGMEM lux_lt[2][24] = {
	 {0xf8, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x18, 0x78, 0xe0, 0xe0, 0x78, 0x18, 0x00, 0x00, 0x80, 0xc0, 0x60, 0x30, 0x18, 0x08},
	 {0x1f, 0x1f, 0x18, 0x18, 0x18, 0x00, 0x18, 0x1e, 0x07, 0x07, 0x1e, 0x18, 0x00, 0x00, 0x01, 0x03, 0x06, 0x0c, 0x18, 0x10}};

	 static const unsigned char PROGMEM lux_gt[2][24] = {
		 {0xf8, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x18, 0x78, 0xe0, 0xe0, 0x78, 0x18, 0x00, 0x00, 0x18, 0x30, 0x60, 0xc0, 0x80, 0x00},
		 {0x1f, 0x1f, 0x18, 0x18, 0x18, 0x00, 0x18, 0x1e, 0x07, 0x07, 0x1e, 0x18, 0x00, 0x00, 0x18, 0x0c, 0x06, 0x03, 0x01, 0x00}};

		 static const unsigned char PROGMEM sunrise[2][16] = {
			 {0x00, 0x80, 0x80, 0xe0, 0xe0, 0xc0, 0xe0, 0xf8, 0xf8, 0xe0, 0xc0, 0xe0, 0xe0, 0x80, 0x80, 0x00},
			 {0x0c, 0x0d, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0d, 0x0c}};
			 

			 static const unsigned char PROGMEM sunset[2][16] = {
				 {0x18, 0xd8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xd8, 0x18},
				 {0x00, 0x00, 0x00, 0x03, 0x03, 0x01, 0x03, 0x0f, 0x0f, 0x03, 0x01, 0x03, 0x03, 0x00, 0x00, 0x00}};
				 

				 static const unsigned char PROGMEM bulb_off[2][16] = {
					 {0x00, 0x00, 0x00, 0xe0, 0xf8, 0x18, 0x0c, 0x0c, 0x0c, 0x18, 0xf8, 0xe0, 0x00, 0x00, 0x00, 0x00},
					 {0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x0a, 0x2a, 0x0a, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}};

					 static const unsigned char PROGMEM bulb_on[2][16] = {
						 {0x00, 0x00, 0x00, 0xe0, 0xf8, 0xf8, 0xfc, 0xfc, 0xfc, 0xf8, 0xf8, 0xe0, 0x00, 0x00, 0x00, 0x00},
						 {0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x0b, 0x2b, 0x0b, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}};

						 static const unsigned char PROGMEM digit_0_9[22][8] = {
							 {0x00, 0xf0, 0xf8, 0x08, 0xf8, 0xf0, 0x00, 0x00},
							 {0x00, 0x0f, 0x1f, 0x10, 0x1f, 0x0f, 0x00, 0x00},
							 {0x00, 0x00, 0x10, 0xf8, 0xf8, 0x00, 0x00, 0x00},
							 {0x00, 0x00, 0x10, 0x1f, 0x1f, 0x10, 0x00, 0x00},
							 {0x00, 0x10, 0x18, 0x88, 0xf8, 0x70, 0x00, 0x00},
							 {0x00, 0x18, 0x1e, 0x17, 0x19, 0x18, 0x00, 0x00},
							 {0x00, 0x10, 0x98, 0x88, 0xf8, 0x70, 0x00, 0x00},
							 {0x00, 0x08, 0x18, 0x10, 0x1f, 0x0f, 0x00, 0x00},
							 {0x00, 0x00, 0xc0, 0xf0, 0xf8, 0xf8, 0x00, 0x00},
							 {0x00, 0x03, 0x03, 0x02, 0x1f, 0x1f, 0x00, 0x00},
							 {0x00, 0xf8, 0xf8, 0x88, 0x88, 0x08, 0x00, 0x00},
							 {0x00, 0x08, 0x18, 0x10, 0x1f, 0x0f, 0x00, 0x00},
							 {0x00, 0xe0, 0xf0, 0x98, 0x88, 0x08, 0x00, 0x00},
							 {0x00, 0x0f, 0x1f, 0x10, 0x1f, 0x0f, 0x00, 0x00},
							 {0x00, 0x18, 0x18, 0xc8, 0xf8, 0x38, 0x00, 0x00},
							 {0x00, 0x00, 0x1e, 0x1f, 0x01, 0x00, 0x00, 0x00},
							 {0x00, 0x70, 0xf8, 0x88, 0xf8, 0x70, 0x00, 0x00},
							 {0x00, 0x0f, 0x1f, 0x10, 0x1f, 0x0f, 0x00, 0x00},
							 {0x00, 0xf0, 0xf8, 0x08, 0xf8, 0xf0, 0x00, 0x00},
							 {0x00, 0x10, 0x11, 0x19, 0x0f, 0x07, 0x00, 0x00},
							 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
							 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

							 static const unsigned char PROGMEM colon[2][2] = {
								 {0x30, 0x30}, {0x0c, 0x0c}};

								 static const unsigned char PROGMEM slash[2][6] = {
									 {0x00, 0x00, 0x80, 0xe0, 0x78, 0x18}, {0x18, 0x1e, 0x07, 0x01, 0x00, 0x00}};

									 uint8_t oled_buffer[512];

int16_t lux_on_limit;
int16_t lux_off_limit;
int16_t lux;
uint8_t hours_after_sunset;
uint8_t minutes_after_sunset;
uint8_t hours_to_sunrise;
uint8_t minutes_to_sunrise;
void ssd1306_init(void)
{
	ssd1306_command(SSD1306_DISPLAYOFF);                    // 0xAE
	ssd1306_command(SSD1306_SETDISPLAYCLOCKDIV);            // 0xD5
	ssd1306_command(0x80);                                  // the suggested ratio 0x80

	ssd1306_command(SSD1306_SETMULTIPLEX);                  // 0xA8
	ssd1306_command(SSD1306_LCDHEIGHT - 1);

	ssd1306_command(SSD1306_SETDISPLAYOFFSET);              // 0xD3
	ssd1306_command(0x0);                                   // no offset
	ssd1306_command(SSD1306_SETSTARTLINE | 0x0);            // line #0
	ssd1306_command(SSD1306_CHARGEPUMP);                    // 0x8D

	ssd1306_command(0x14);

	ssd1306_command(SSD1306_MEMORYMODE);                    // 0x20
	ssd1306_command(0x00);                                  // 0x0 act like ks0108
	ssd1306_command(SSD1306_SEGREMAP | 0x1);
	ssd1306_command(SSD1306_COMSCANDEC);

	ssd1306_command(SSD1306_SETCOMPINS);                    // 0xDA
	ssd1306_command(0x02);
	ssd1306_command(SSD1306_SETCONTRAST);                   // 0x81
	ssd1306_command(0x8F);

	ssd1306_command(SSD1306_SETPRECHARGE);                  // 0xd9
	ssd1306_command(0xF1);

	ssd1306_command(SSD1306_SETVCOMDETECT);                 // 0xDB
	ssd1306_command(0x40);
	ssd1306_command(SSD1306_DISPLAYALLON_RESUME);           // 0xA4
	ssd1306_command(SSD1306_NORMALDISPLAY);                 // 0xA6

	ssd1306_command(SSD1306_DEACTIVATE_SCROLL);

	ssd1306_command(SSD1306_DISPLAYON);//--turn on oled panel
}

void ssd1306_command(uint8_t command)
{
	i2c_start(SSD1306_ADDRESS+I2C_WRITE);
	i2c_write(0);
	i2c_write(command);
	i2c_stop();
}

void ssd1306_display(void)
{
	ssd1306_command(SSD1306_COLUMNADDR);
	ssd1306_command(0);   // Column start address (0 = reset)
	ssd1306_command(SSD1306_LCDWIDTH-1); // Column end address (127 = reset)
	ssd1306_command(SSD1306_PAGEADDR);
	ssd1306_command(0); // Page start address (0 = reset)
	ssd1306_command(3); // Page end address

	for (uint16_t i=0; i<(SSD1306_LCDWIDTH*SSD1306_LCDHEIGHT/8); i++)
	{
		i2c_start(SSD1306_ADDRESS+I2C_WRITE);
		i2c_write(0x40);
		for (uint8_t x=0; x<16; x++)
		{
			i2c_write(oled_buffer[i]);
			i++;
		}
		i--;
		i2c_stop();
	}
}

void display_sunset(void)
{
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < 16; j++) oled_buffer[SUNSET_X+i*128+j+SUNSET_Y*16] = pgm_read_byte(&(sunset[i][j]));
	}
}

void display_sunrise(void)
{
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < 16; j++) oled_buffer[SUNRISE_X+i*128+j+SUNRISE_Y*16] = pgm_read_byte(&(sunrise[i][j]));
	}
}

void display_time_after_sunset(uint8_t blink)
{
	{
		for(uint8_t i=0; i<2; i++)
		{
			for (uint8_t j = 0; j < 16; j++) oled_buffer[TIME_AFTER_SUNSET_SUNSET_X+i*128+j+TIME_AFTER_SUNSET_Y*16] = pgm_read_byte(&(sunset[i][j]));
		}
	}
	
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[TIME_AFTER_SUNSET_HOUR_X+i*128+j+TIME_AFTER_SUNSET_Y*16] = pgm_read_byte(&(digit_0_9[2*(hours_after_sunset)+i][j]));
	}
	
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < 2; j++) oled_buffer[TIME_AFTER_SUNSET_COLON_X+i*128+j+TIME_AFTER_SUNSET_Y*16] = pgm_read_byte(&(colon[i][j]));
	}
	
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[TIME_AFTER_SUNSET_TEN_MINUTE_X+i*128+j+TIME_AFTER_SUNSET_Y*16] = pgm_read_byte(&(digit_0_9[2*(minutes_after_sunset/10)+i][j]));
	}

	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[TIME_AFTER_SUNSET_ONE_MINUTE_X+i*128+j+TIME_AFTER_SUNSET_Y*16] = pgm_read_byte(&(digit_0_9[2*(minutes_after_sunset%10)+i][j]));
	}

	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < BULB_WIDTH; j++) oled_buffer[TIME_AFTER_SUNSET_BULB_OFF_X+i*128+j+TIME_AFTER_SUNSET_Y*16] = pgm_read_byte(&(bulb_off[i][j]));
	}

	switch (blink)
	{
		case sunset_hour:
		for(uint8_t i=0; i<2; i++)
		{
			for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[TIME_AFTER_SUNSET_HOUR_X+i*128+j+TIME_AFTER_SUNSET_Y*16] = 0;
		}
		break;
		
		case sunset_minute:
		for(uint8_t i=0; i<2; i++)
		{
			for (uint8_t j = 0; j < DIGIT_WIDTH*2; j++) oled_buffer[TIME_AFTER_SUNSET_TEN_MINUTE_X+i*128+j+TIME_AFTER_SUNSET_Y*16] = 0;
		}
		break;
	}
}

void display_time_to_sunrise(uint8_t blink)
{
	{
		for(uint8_t i=0; i<2; i++)
		{
			for (uint8_t j = 0; j < 16; j++) oled_buffer[TIME_TO_SUNRISE_SUNRISE_X+i*128+j+TIME_TO_SUNRISE_Y*16] = pgm_read_byte(&(sunrise[i][j]));
		}
	}

	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[TIME_TO_SUNRISE_HOUR_X+i*128+j+TIME_TO_SUNRISE_Y*16] = pgm_read_byte(&(digit_0_9[2*(hours_to_sunrise)+i][j]));
	}
	
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < 2; j++) oled_buffer[TIME_TO_SUNRISE_COLON_X+i*128+j+TIME_TO_SUNRISE_Y*16] = pgm_read_byte(&(colon[i][j]));
	}
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[TIME_TO_SUNRISE_TEN_MINUTE_X+i*128+j+TIME_TO_SUNRISE_Y*16] = pgm_read_byte(&(digit_0_9[2*(minutes_to_sunrise/10)+i][j]));
	}
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[TIME_TO_SUNRISE_ONE_MINUTE_X+i*128+j+TIME_TO_SUNRISE_Y*16] = pgm_read_byte(&(digit_0_9[2*(minutes_to_sunrise%10)+i][j]));
	}
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < BULB_WIDTH; j++) oled_buffer[TIME_TO_SUNRISE_BULB_ON_X+i*128+j+TIME_TO_SUNRISE_Y*16] = pgm_read_byte(&(bulb_on[i][j]));
	}
	switch (blink)
	{
		case sunrise_hour:
		
		for(uint8_t i=0; i<2; i++)
		{
			for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[TIME_TO_SUNRISE_HOUR_X+i*128+j+TIME_TO_SUNRISE_Y*16] = 0;
		}
		break;
		
		case sunrise_minute:
		for(uint8_t i=0; i<2; i++)
		{
			for (uint8_t j = 0; j < DIGIT_WIDTH*2; j++) oled_buffer[TIME_TO_SUNRISE_TEN_MINUTE_X+i*128+j+TIME_TO_SUNRISE_Y*16] = 0;
		}
		break;
	}
}

void display_sunrise_time(int8_t sunRiseTime_hour, int8_t sunRiseTime_minute)
{
	{
		for(uint8_t i=0; i<2; i++)
		{
			for (uint8_t j = 0; j < 16; j++) oled_buffer[SUNRISE_SUN_X+i*128+j+SUNRISE_Y*16] = pgm_read_byte(&(sunrise[i][j]));
		}
	}

	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[SUNRISE_HOUR_X+i*128+j+SUNRISE_Y*16] = pgm_read_byte(&(digit_0_9[2*(sunRiseTime_hour%10)+i][j]));
	}
	
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < 2; j++) oled_buffer[SUNRISE_COLON_X+i*128+j+SUNRISE_Y*16] = pgm_read_byte(&(colon[i][j]));
	}
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[SUNRISE_TEN_MINUTE_X+i*128+j+SUNRISE_Y*16] = pgm_read_byte(&(digit_0_9[2*(sunRiseTime_minute/10)+i][j]));
	}
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[SUNRISE_ONE_MINUTE_X+i*128+j+SUNRISE_Y*16] = pgm_read_byte(&(digit_0_9[2*(sunRiseTime_minute%10)+i][j]));
	}
}

void display_sunset_time(int8_t sunSetTime_hour, int8_t sunSetTime_minute)
{
	{
		for(uint8_t i=0; i<2; i++)
		{
			for (uint8_t j = 0; j < 16; j++) oled_buffer[SUNSET_SUN_X+i*128+j+SUNSET_Y*16] = pgm_read_byte(&(sunset[i][j]));
		}
	}

	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[SUNSET_TEN_HOUR_X+i*128+j+SUNSET_Y*16] = pgm_read_byte(&(digit_0_9[2*(sunSetTime_hour/10)+i][j]));
	}

	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[SUNSET_ONE_HOUR_X+i*128+j+SUNSET_Y*16] = pgm_read_byte(&(digit_0_9[2*(sunSetTime_hour%10)+i][j]));
	}
	
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < 2; j++) oled_buffer[SUNSET_COLON_X+i*128+j+SUNSET_Y*16] = pgm_read_byte(&(colon[i][j]));
	}
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[SUNSET_TEN_MINUTE_X+i*128+j+SUNSET_Y*16] = pgm_read_byte(&(digit_0_9[2*(sunSetTime_minute/10)+i][j]));
	}
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[SUNSET_ONE_MINUTE_X+i*128+j+SUNSET_Y*16] = pgm_read_byte(&(digit_0_9[2*(sunSetTime_minute%10)+i][j]));
	}
}

void display_lux(uint8_t blink)
{
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < LUX_LT_WIDTH; j++)	oled_buffer[LUX_LT_X+i*128+j+LUX_LT_Y*16] = pgm_read_byte(&(lux_lt[i][j]));
	}

	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < BULB_WIDTH; j++) oled_buffer[LUX_BULB_ON_X+i*128+j+LUX_BULB_ON_Y*16] = pgm_read_byte(&(bulb_on[i][j]));
	}

	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < LUX_GT_WIDTH; j++) oled_buffer[LUX_GT_X+i*128+j+LUX_GT_Y*16] = pgm_read_byte(&(lux_gt[i][j]));
	}

	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < BULB_WIDTH; j++) oled_buffer[LUX_BULB_OFF_X+i*128+j+LUX_BULB_OFF_Y*16] = pgm_read_byte(&(bulb_off[i][j]));
	}
	
	for(uint8_t i=0; i<NUM_BYTES_HEIGHT; i++)
	{
		for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[LUX_ON_HUNDRED_X+i*128+j+LUX_ON_Y*16] = pgm_read_byte(&(digit_0_9[2*(lux_on_limit/100)+i][j]));
	}

	for(uint8_t i=0; i<NUM_BYTES_HEIGHT; i++)
	{
		for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[LUX_ON_TEN_X+i*128+j+LUX_ON_Y*16] = pgm_read_byte(&(digit_0_9[2*((lux_on_limit %100)/10)+i][j]));
	}

	for(uint8_t i=0; i<NUM_BYTES_HEIGHT; i++)
	{
		for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[LUX_ON_ONE_X+i*128+j+LUX_ON_Y*16] = pgm_read_byte(&(digit_0_9[2*(lux_on_limit % 10)+i][j]));
	}

	for(uint8_t i=0; i<NUM_BYTES_HEIGHT; i++)
	{
		for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[LUX_OFF_HUNDRED_X+i*128+j+LUX_OFF_Y*16] = pgm_read_byte(&(digit_0_9[2*(lux_off_limit/100)+i][j]));
	}

	for(uint8_t i=0; i<NUM_BYTES_HEIGHT; i++)
	{
		for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[LUX_OFF_TEN_X+i*128+j+LUX_OFF_Y*16] = pgm_read_byte(&(digit_0_9[2*((lux_off_limit %100)/10)+i][j]));
	}

	for(uint8_t i=0; i<NUM_BYTES_HEIGHT; i++)
	{
		for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[LUX_OFF_ONE_X+i*128+j+LUX_OFF_Y*16] = pgm_read_byte(&(digit_0_9[2*(lux_off_limit % 10)+i][j]));
	}

	switch(blink)
	{
		case lx_lt_blink:
		
		for(uint8_t i=0; i<NUM_BYTES_HEIGHT; i++)
		{
			for (uint8_t j = 0; j < DIGIT_WIDTH*3; j++) oled_buffer[LUX_ON_HUNDRED_X+i*128+j+LUX_ON_Y*16] = 0;
			
		}
		break;

		case lx_gt_blink:
		
		for(uint8_t i=0; i<NUM_BYTES_HEIGHT; i++)
		{
			for (uint8_t j = 0; j < DIGIT_WIDTH*3; j++) oled_buffer[LUX_OFF_HUNDRED_X+i*128+j+LUX_OFF_Y*16] = 0;
		}
		break;
	}
}

void display_lux_val(uint16_t val)
{
	uint8_t hun, ten;
	hun = val/100;
	ten = (val%100)/10;
	if (hun > 0)
	{
		for(uint8_t i=0; i<NUM_BYTES_HEIGHT; i++)
		{
			for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[ADC_HUNDRED_X+i*128+j+ADC_Y*16] = pgm_read_byte(&(digit_0_9[2*hun+i][j]));
		}
		for(uint8_t i=0; i<NUM_BYTES_HEIGHT; i++)
		{
			for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[ADC_TEN_X+i*128+j+ADC_Y*16] = pgm_read_byte(&(digit_0_9[2*ten+i][j]));
		}
	}
	else if (ten > 0)
	{
		for(uint8_t i=0; i<NUM_BYTES_HEIGHT; i++)
		{
			for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[ADC_TEN_X+i*128+j+ADC_Y*16] = pgm_read_byte(&(digit_0_9[2*ten+i][j]));
		}
	}
	for(uint8_t i=0; i<NUM_BYTES_HEIGHT; i++)
	{
		for (uint8_t j = 0; j < DIGIT_WIDTH; j++) oled_buffer[ADC_ONE_X+i*128+j+ADC_Y*16] = pgm_read_byte(&(digit_0_9[2*(val % 10)+i][j]));
	}
}

void display_date_time(int8_t day, int8_t month, int16_t year, int8_t hour, int8_t minute, int8_t second)
{
	int8_t one_second, ten_second, one_minute, ten_minute, one_hour, ten_hour;
	int8_t one_day, ten_day, one_month, ten_month, one_year, ten_year;

	ten_second = second/10;
	one_second = second % 10;
	ten_hour = hour/10;
	one_hour = hour % 10;
	ten_minute = minute/10;
	one_minute = minute % 10;
	ten_day = day/10;
	one_day = day % 10;
	ten_month = (month + 1)/10;
	one_month = (month + 1) % 10;
	ten_year = (year - 100)/10;
	one_year = (year - 100) % 10;
	//	printf("year %d \n", year);
	/**/
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < 8; j++)
		{
			oled_buffer[DAY_TEN_X+i*128+j+DATE_Y*16] = pgm_read_byte(&(digit_0_9[2*ten_day+i][j]));
		}
	}
	
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < 8; j++)
		{
			oled_buffer[DAY_ONE_X+i*128+j+DATE_Y*16] = pgm_read_byte(&(digit_0_9[2*one_day+i][j]));
		}
	}
	
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < 8; j++)
		{
			oled_buffer[MONTH_TEN_X+i*128+j+DATE_Y*16] = pgm_read_byte(&(digit_0_9[2*ten_month+i][j]));
		}
	}
	
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < 8; j++)
		{
			oled_buffer[MONTH_ONE_X+i*128+j+DATE_Y*16] = pgm_read_byte(&(digit_0_9[2*one_month+i][j]));
		}
	}
	
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < 8; j++)
		{
			oled_buffer[YEAR_TEN_X+i*128+j+DATE_Y*16] = pgm_read_byte(&(digit_0_9[2*ten_year+i][j]));
		}
	}
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < 8; j++)
		{
			oled_buffer[YEAR_ONE_X+i*128+j+DATE_Y*16] = pgm_read_byte(&(digit_0_9[2*one_year+i][j]));
		}
	}

	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < 6; j++)
		{
			oled_buffer[DAY_SLASH_X+i*128+j+DATE_Y*16] = pgm_read_byte(&(slash[i][j]));
		}
	}
	
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < 6; j++)
		{
			oled_buffer[MONTH_SLASH_X+i*128+j+DATE_Y*16] = pgm_read_byte(&(slash[i][j]));
		}
	}

	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < 8; j++)
		{
			oled_buffer[SECOND_TEN_X+i*128+j+DATE_Y*16] = pgm_read_byte(&(digit_0_9[2*ten_second+i][j]));
		}
	}
	
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < 8; j++)
		{
			oled_buffer[SECOND_ONE_X+i*128+j+DATE_Y*16] = pgm_read_byte(&(digit_0_9[2*one_second+i][j]));
		}
	}
	
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < 8; j++)
		{
			oled_buffer[MINUTE_TEN_X+i*128+j+DATE_Y*16] = pgm_read_byte(&(digit_0_9[2*ten_minute+i][j]));
		}
	}
	
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < 8; j++)
		{
			oled_buffer[MINUTE_ONE_X+i*128+j+DATE_Y*16] = pgm_read_byte(&(digit_0_9[2*one_minute+i][j]));
		}
	}
	
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < 8; j++)
		{
			oled_buffer[HOUR_TEN_X+i*128+j+DATE_Y*16] = pgm_read_byte(&(digit_0_9[2*ten_hour+i][j]));
		}
	}
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < 8; j++)
		{
			oled_buffer[HOUR_ONE_X+i*128+j+DATE_Y*16] = pgm_read_byte(&(digit_0_9[2*one_hour+i][j]));
		}
	}

	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < 2; j++)
		{
			oled_buffer[HOUR_COLON_X+i*128+j+DATE_Y*16] = pgm_read_byte(&(colon[i][j]));
		}
	}
	
	for(uint8_t i=0; i<2; i++)
	{
		for (uint8_t j = 0; j < 2; j++)
		{
			oled_buffer[MINUTE_COLON_X+i*128+j+DATE_Y*16] = pgm_read_byte(&(colon[i][j]));
		}
	}
}
