/*
 * defines.h
 *
 * Created: 25/08/2022 10:14:08
 *  Author: Peter
 */ 


/*
 * defines.h
 *
 * Created: 13/01/2018 16:24:14
 *  Author: Peter
 */ 


#ifndef DEFINES_H_
#define DEFINES_H_

#define IGNORE_CHECKSUM 0
#define TEST_GMT 0
#define USE_OLED 0

#define PRIO_1 0
#define PRIO_2 1
#define PRIO_3 2
#define PRIO_4 3
#define PRIO_5 4
#define PRIO_6 5
#define PRIO_7 6
#define PRIO_8 7

#define TROTTLE 511
#define SOUTH_INIT_IX 0
#define SOUTH_TRACKING_IX 1

#define SUNRISE_SUN_X 0
#define SUNRISE_HOUR_X 16
#define SUNRISE_COLON_X 24
#define SUNRISE_TEN_MINUTE_X 28
#define SUNRISE_ONE_MINUTE_X 36
#define SUNRISE_Y 16
#define SUNSET_SUN_X 50
#define SUNSET_TEN_HOUR_X 66
#define SUNSET_ONE_HOUR_X 74
#define SUNSET_COLON_X 82
#define SUNSET_TEN_MINUTE_X 86
#define SUNSET_ONE_MINUTE_X 94
#define ADC_HUNDRED_X 104
#define ADC_TEN_X 112
#define ADC_ONE_X 120
#define SUNSET_Y 16

#define ADC_Y 16
#define SUNSET_X 0
#define DIG1_X 23
#define DIG1_Y 0
#define COLON1_X 33
#define COLON1_Y 0
#define DIG2_X 36
#define DIG2_Y 0
#define DIG3_X 44
#define DIG3_Y 0
#define BULB_ON1_X 56
#define BULB_ON1_Y 0
#define DIG4_X 76
#define DIG4_Y 0
#define COLON2_X 86
#define COLON2_Y 0
#define DIG5_X 89
#define DIG5_Y 0
#define DIG6_X 97
#define DIG6_Y 0
#define BULB_OFF1_X 112
#define BULB_OFF1_Y 0

#define BULB_ON2_X 1
#define BULB_ON2_Y 16
#define DIG7_X 23
#define DIG7_Y 16
#define COLON3_X 33
#define COLON3_Y 16
#define DIG8_X 36
#define DIG8_Y 16
#define DIG9_X 44
#define DIG9_Y 16
#define BULB_OFF2_X 56
#define BULB_OFF2_Y 16
#define DIG10_X 76
#define DIG10_Y 16
#define COLON4_X 86
#define COLON4_Y 16
#define DIG11_X 89
#define DIG11_Y 16
#define DIG12_X 97
#define DIG12_Y 16
#define SUNRISE_X 112
#define SUNRISE_Y 16
#define HOUR_TEN_X 68
#define DATE_Y 0
#define HOUR_ONE_X 76
#define HOUR_COLON_X 86
#define MINUTE_TEN_X 90
#define MINUTE_ONE_X 98
#define MINUTE_COLON_X 108
#define SECOND_TEN_X 112
#define SECOND_ONE_X 120
#define DAY_TEN_X 0
#define DAY_ONE_X 8
#define DAY_SLASH_X 16
#define MONTH_TEN_X 22
#define MONTH_ONE_X 30
#define MONTH_SLASH_X 38
#define YEAR_TEN_X 44
#define YEAR_ONE_X 52
#define DIGIT_WIDTH 8
#define NUM_BYTES_HEIGHT 2
#define LUX_LT_WIDTH 24
#define LUX_GT_WIDTH 24
#define LUX_LT_X 0
#define LUX_LT_Y 0
#define LUX_GT_X 68
#define LUX_GT_Y 16

#define BULB_WIDTH 16
#define SUN_WITDH 16
#define LUX_BULB_ON_X 44
#define LUX_BULB_ON_Y 0
#define LUX_BULB_OFF_X 112
#define LUX_BULB_OFF_Y 16

#define LUX_ON_HUNDRED_X 20
#define LUX_ON_Y 0
#define LUX_ON_TEN_X 28
#define LUX_ON_ONE_X 36
#define LUX_OFF_HUNDRED_X 88
#define LUX_OFF_Y 16
#define LUX_OFF_TEN_X 96
#define LUX_OFF_ONE_X 104

#define TIME_TO_SUNRISE_SUNRISE_X 112
#define TIME_TO_SUNRISE_HOUR_X 84
#define TIME_TO_SUNRISE_COLON_X 93
#define TIME_TO_SUNRISE_TEN_MINUTE_X 96
#define TIME_TO_SUNRISE_ONE_MINUTE_X 104
#define TIME_TO_SUNRISE_BULB_ON_X 68
#define TIME_TO_SUNRISE_Y 0

#define TIME_AFTER_SUNSET_SUNSET_X 0
#define TIME_AFTER_SUNSET_HOUR_X 16
#define TIME_AFTER_SUNSET_COLON_X 25
#define TIME_AFTER_SUNSET_TEN_MINUTE_X 28
#define TIME_AFTER_SUNSET_ONE_MINUTE_X 36
#define TIME_AFTER_SUNSET_BULB_OFF_X 44
#define TIME_AFTER_SUNSET_Y 16

#define  SSD1306_ADDRESS 0x78
#define SSD1306_LCDWIDTH 128
#define SSD1306_LCDHEIGHT 32
#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF

#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA

#define SSD1306_SETVCOMDETECT 0xDB

#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9

#define SSD1306_SETMULTIPLEX 0xA8

#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10

#define SSD1306_SETSTARTLINE 0x40

#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR   0x22

#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8

#define SSD1306_SEGREMAP 0xA0

#define SSD1306_CHARGEPUMP 0x8D

#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2

// Scrolling #defines
#define SSD1306_ACTIVATE_SCROLL 0x2F
#define SSD1306_DEACTIVATE_SCROLL 0x2E
#define SSD1306_SET_VERTICAL_SCROLL_AREA 0xA3
#define SSD1306_RIGHT_HORIZONTAL_SCROLL 0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL 0x27
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL 0x2A

#define GLUE(a, b)     a##b

/* single-bit macros, used for control bits */
#define SET_(what, p, m) GLUE(what, p) |= (1 << (m))
#define CLR_(what, p, m) GLUE(what, p) &= ~(1 << (m))
#define GET_(/* PIN, */ p, m) GLUE(PIN, p) & (1 << (m))
#define SET(what, x) SET_(what, x)
#define CLR(what, x) CLR_(what, x)
#define GET(/* PIN, */ x) GET_(x)

/* nibble macros, used for data path */
#define ASSIGN_(what, p, m, v) GLUE(what, p) = (GLUE(what, p) & \
~((1 << (m)) | (1 << ((m) + 1)) | \
(1 << ((m) + 2)) | (1 << ((m) + 3)))) | \
((v) << (m))
#define READ_(what, p, m) (GLUE(what, p) & ((1 << (m)) | (1 << ((m) + 1)) | \
(1 << ((m) + 2)) | (1 << ((m) + 3)))) >> (m)
#define ASSIGN(what, x, v) ASSIGN_(what, x, v)
#define READ(what, x) READ_(what, x)

#define UCSRA UCSR0A
#define UBRR UBRR0 
#define UBRRL UBRR0L
#define UBRRH UBRR0H
#define UDR UDR0
#define UCSRB UCSR0B
#define TXEN TXEN0
#define RXEN RXEN0
#define UDRE UDRE0
#define RXC RXC0
#define FE FE0
#define DOR DOR0
#define U2X U2X0

#define ELKO_TRIAC C, 1
#define ELKO_RELAY C, 2

#define RED_LED B, 3
#define BLUE_LED B, 4

#define DEMO_WEDGE B, 5
#define DEMO_WEDGE_PORT PINB
#define DEMO_WEDGE_PIN PINB5

#define FIX_PORT PINB
#define FIX_PIN PINB0

#define ENC_PUSH_SW D, 7
#define ENC_PUSH_SW_PORT PIND
#define ENC_PUSH_SW_PIN PIND7

//#define TIME_ZONE 2
//#define JULIAN_DAY_2017_12_31 2458119.0F
//#define JULIAN_DAY_2017_12_31 2458118.5F + 0.5F - TIME_ZONE/24 /* Julian Day */
//#define JULIAN_DAY_REF 2415018.5F /* Julian Day */
#define ECCENT_EARTH_ORBIT 0.0167F
#define K2_CONST 1.000001018F * (1.0F - ECCENT_EARTH_ORBIT * ECCENT_EARTH_ORBIT)

#define ON_HYST_COUNTER 0
#define OFF_HYST_COUNTER 40;
#define DEMO_HYST_COUNTER 2

#define DUSK_RELAY_ON_OFFSET -60

/* Longest time the motor may run without reaching a sensor (INT0 revolution pulse, south sensor, end stop) */
#define STALL_TIMEOUT_S 120
#endif /* DEFINES_H_ */