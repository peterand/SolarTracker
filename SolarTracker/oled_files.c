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

void calcSunRiseSunSet(uint8_t * sunRiseTime_hour, uint8_t * sunRiseTime_minute, uint8_t * sunSetTime_hour, uint8_t * sunSetTime_minute, int res_nLatitudeDegrees, int res_nLongitudeDegrees, uint8_t time_zone, float JulianDay)
{
	float JulianCentury;
	float GeomMeanLongSun_deg;
	float GeomMeanAnomSun_deg;
	float SunEqOfCtr;
	float SunTrueLong_deg;
	float SunTrueAnom_deg;
	float SunAppLong_deg;
	float MeanObliqEcliptic_deg;
	float ObliqCorr_deg;
	float SunDeclin_deg;
	float EqOfTime_minutes;
	float var_y;
	float HA_Sunrise_deg;
	float SolarNoon;
//	uint16_t iSunRiseTime;
//	uint16_t iSunSetTime;
//	uint16_t iSolarNoon;

	float f1;
	float f2;
	float f3;
	float f4;
	float f5;
	float f6;
	float f7;
	float f8;

	JulianCentury = JulianDay/36525.0F; 
	//GeomMeanLongSun_deg = MOD(280.46646+G2*(36000.76983 + G2*0.0003032),360)

	f1 = JulianCentury * 0.0003032F;
	f2 = 36000.76983F + f1;
	f3 = JulianCentury * f2;
	f4 = 280.46646F + f3;
	if (f4 > 360.0F)
	{
		for (;;)
		{
			f4 -= 360.0F;
			if (f4 < 360.0F) break;
		}
	}
	GeomMeanLongSun_deg = f4; //I2
	//  Geom Mean Anom Sun (deg) = 357.52911+G2*(35999.05029 - 0.0001537*G2)	
	f1 = 0.0001537F * JulianCentury;
	f2 = 35999.05029F - f1;
	f3 = JulianCentury * f2;
	f4 = 357.52911 + f3;
	GeomMeanAnomSun_deg = f4; //J2
   /*
	*               <---------------------------f2--------------------->
	*                                 <-----------------f3------------->  <---------------------------------f4-------------------------------->
	*                                          <-----------f2---------->  <--------------------f3---------------->
	*                                              <--------f3--------->                      <--------f4-------->  <------------f2----------->
	*               <------f1----->                         <----f2---->  <-------f1----->             <----f3--->  <------f1------->
	*  SunEqOfCtr = SIN(RADIANS(J2))*(1.914602-G2*(0.004817+0.000014*G2))+SIN(RADIANS(2*J2))*(0.019993-0.000101*G2)+SIN(RADIANS(3*J2))*0.000289
	*/
	f1 = sin(M_PI / 60 * GeomMeanAnomSun_deg);
	f2 = f1 * 0.000289F;
	f3 = 0.000101F * JulianCentury;
	f4 = 0.019993F - f3;
	f1 = sin(M_PI / 90 * GeomMeanAnomSun_deg);
	f3 = f1 * f4;
	f4 = f3 + f2;
	f1 = sin(M_PI / 180 * GeomMeanAnomSun_deg);
	f2 = 0.000014F * JulianCentury;
	f3 = 0.004817F + f2;
	f2 = JulianCentury * f3;
	f3 = 1.914602F - f2;
	f2 = f1 * f3;
	f1 = f2 + f4;
	SunEqOfCtr = f1; // L2
	//	Sun True Long (deg) = I2 + L2
	SunTrueLong_deg = GeomMeanLongSun_deg + SunEqOfCtr; //M2

	//	Sun True Anom (deg) = J2 + L2
	SunTrueAnom_deg = GeomMeanAnomSun_deg + SunEqOfCtr; //N2

	//  K2 = ECCENT_EARTH_ORBIT
	//	Sun Rad Vector (AUs) = (1.000001018*(1-K2*K2))/(1+K2*COS(RADIANS(N2)))

	f1 = cos(M_PI / 180 * SunTrueAnom_deg);
	f2 = ECCENT_EARTH_ORBIT * f1 + 1.0F;
	f3 = K2_CONST / f2;
	//	SunRadVector_AUs = f3; //O2

	// Sun App Long (deg) = M2-0.00569-0.00478*SIN(RADIANS(125.04-1934.136*G2))
	f1 = 1934.136F * JulianCentury;
	f2 = 125.04F - f1;
	f3 = sin(M_PI / 180 * f2);
	f4 = 0.00478F * f3;
	f1 = SunTrueLong_deg - 0.00569F;
	f2 = f1 - f4;
	SunAppLong_deg = f2;  //P2

	//Mean Obliq Ecliptic (deg) = 23+(26+((21.448-G2*(46.815+G2*(0.00059-G2*0.001813))))/60)/60
	f1 = JulianCentury * 0.001813F;
	f2 = 0.00059F - f1;
	f3 = JulianCentury * f2;
	f4 = 46.815F + f3;
	f1 = JulianCentury * f4;
	f2 = 21.448F - f1;
	f3 = f2 / 60.0F;
	f4 = 26.0F + f3;
	f1 = f4 / 60.0F;
	f2 = 23.0F + f1;
	MeanObliqEcliptic_deg = f2; //Q2

	// Obliq Corr (deg) = Q2+0.00256*COS(RADIANS(125.04-1934.136*G2))
	f1 = 1934.136F * JulianCentury;
	f2 = 125.04F - f1;
	f3 = cos(M_PI / 180 * f2);
	f4 = 0.00256F * f3;
	f1 = MeanObliqEcliptic_deg + f4;
	ObliqCorr_deg = f1;		//R2	

	//Sun Rt Ascen (deg) = DEGREES(ATAN2(COS(RADIANS(P2)),COS(RADIANS(R2))*SIN(RADIANS(P2))))
	f1 = sin(M_PI / 180 * SunAppLong_deg);
	f2 = cos(M_PI / 180 * ObliqCorr_deg);
	f3 = f1 * f2;
	f4 = cos(M_PI / 180 * SunAppLong_deg);
	f1 = atan2(f3, f4);	// Reverse order compared to Excel
	f2 = f1 * 180 / M_PI;
	//	SunRtAscen_deg = f2; //S2

	//Sun Declin (deg) =DEGREES(ASIN(SIN(RADIANS(R2))*SIN(RADIANS(P2)))) 
	f1 = sin(M_PI / 180 * SunAppLong_deg);
	f2 = sin(M_PI / 180 * ObliqCorr_deg);
	f3 = f1 * f2;
	f4 = asin(f3);
	f1 = 180 / M_PI * f4;
	SunDeclin_deg = f1; //T2

	//var y = TAN(RADIANS(R2/2))*TAN(RADIANS(R2/2))
	f1 = tan(M_PI / 360 * ObliqCorr_deg);
	f2 = f1 * f1;
	var_y = f2;			//U2
   /*
	*                       <------------------------------f1------------------->
	*                                1      2         3  32         2       3  32            2       3 3 2    2         3  32              2         3  32               2         3  321
	*Eq of Time (minutes) = 4*DEGREES(U2*SIN(2*RADIANS(I2))-2*K2*SIN(RADIANS(J2))+4*K2*U2*SIN(RADIANS(J2))*COS(2*RADIANS(I2))-0.5*U2*U2*SIN(4*RADIANS(I2))-1.25*K2*K2*SIN(2*RADIANS(J2)))
	*/
	f1 = sin(M_PI / 90 * GeomMeanLongSun_deg);
	f2 = f1 * var_y;
	f3 = sin(M_PI / 180 * GeomMeanAnomSun_deg);
	f4 = f3 * ECCENT_EARTH_ORBIT;
	f5 = f4 * 2.0F;
	f1 = f2 - f5;  //delsumma 1
	f2 = cos(M_PI / 90 * GeomMeanLongSun_deg);
	f3 = sin(M_PI / 180 * GeomMeanAnomSun_deg);
	f4 = f2 * f3;
	f2 = var_y * f4;
	f3 = f2 * ECCENT_EARTH_ORBIT;
	f4 = 4.0F * f3;
	f2 = f1 + f4; //delsumma 2
	f1 = sin(M_PI / 45 * GeomMeanLongSun_deg);
	f3 = f1 * var_y;
	f4 = f3 * var_y;
	f5 = 0.5F * f4;
	f3 = f2 - f5; //delsumma3
	f1 = sin(M_PI / 90 * GeomMeanAnomSun_deg);
	f2 = f1 * ECCENT_EARTH_ORBIT;
	f4 = f2 * ECCENT_EARTH_ORBIT;
	f5 = f4 * 1.25F;
	f4 = f3 - f5; //delsumma 4
	f1 = f4 * 720.0F / M_PI;
	EqOfTime_minutes = f1; //V2
	//                          1    2   3       4      43 3   4       5   5 4    4       5  543    3       4    43    3       4  4321 
	//HA Sunrise (deg) = DEGREES(ACOS(COS(RADIANS(90.833))/(COS(RADIANS($B$3))*COS(RADIANS(T2)))-TAN(RADIANS($B$3))*TAN(RADIANS(T2))))
	f1 = cos(M_PI / 180 * 90.833F);
	//f2 = cos(M_PI / 180 * Latitude_deg);
	f2 = cos(M_PI / 180 * (float)res_nLatitudeDegrees);
	f3 = cos(M_PI / 180 * SunDeclin_deg);
	//f4 = tan(M_PI / 180 * Latitude_deg);
	f4 = tan(M_PI / 180 * (float)res_nLatitudeDegrees);
	f5 = tan(M_PI / 180 * SunDeclin_deg);
	f6 = f2 * f3;
	f7 = f1 / f6;
	f8 = f4 * f5;
	f1 = f7 - f8;
	f2 = 180 / M_PI * acos(f1);
	HA_Sunrise_deg = f2; //W2
	//printf("HA_Sunrise_deg %f\n", HA_Sunrise_deg);

	//Solar Noon (LST) = (720-4*$B$4-V2+$B$5*60)/1440
	//$B$4 Longitude
	//$B$5 TimeZone
	//V2 EqOfTime_minutes
	//f1 = 4 * Longitude_deg;
	//f1 = 4 * (float)res_nLongitudeDegrees;
	//f2 = TimeZone * 60;
	//f2 = TIME_ZONE * 60;
	//f3 = EqOfTime_minutes + f2;
	//f4 = 720 - f1 - f3;
	//f5 = f4 / 1440;
	//	SolarNoon f1 %f,  f5; //X2
	SolarNoon = (720.0F-4*(float)res_nLongitudeDegrees-EqOfTime_minutes+time_zone*60)/1440;
	//printf("EqOfTime_minutes %f, SolarNoon %f\n",EqOfTime_minutes,SolarNoon);
	//Sunrise Time (LST) = X2-W2*4/1440
	f1 = SolarNoon - HA_Sunrise_deg/360;
	//printf("f1 %f\n",f1);
	//f1 = SolarNoon - HA_Sunrise_deg * 4;
	//SunRiseTime = f1;
	iSunRiseTime = (uint16_t)(f1*1440);
	*sunRiseTime_hour = iSunRiseTime / 60;
	*sunRiseTime_minute = iSunRiseTime % 60;

	//Sunset Time (LST)	= X2+W2*4/1440
	f2 = SolarNoon + HA_Sunrise_deg/360;
	//SunSetTime = f2;
	iSunSetTime = (uint16_t)(f2*1440);
	*sunSetTime_hour = iSunSetTime / 60;
	*sunSetTime_minute = iSunSetTime % 60;
	iSolarNoon = (uint16_t)(SolarNoon*1440);
//	solarNoon_hour = iSolarNoon / 60;
//	solarNoon_minute = iSolarNoon % 60;
//	printf("SunRise: %d:%d, SunSet: %d:%d\n",sunRiseTime_hour, sunRiseTime_minute, sunSetTime_hour, sunSetTime_minute);
//	printf("SolaNoon: %d:%d %d, \n",solarNoon_hour, solarNoon_minute, time_zone);
}
