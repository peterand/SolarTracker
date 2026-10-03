/*
 * oled_fles.h
 *
 * Created: 06/10/2022 20:56:10
 *  Author: Peter
 */ 


#ifndef OLED_FLES_H_
#define OLED_FLES_H_

#include <stdint.h>

void ssd1306_init(void);
void ssd1306_command(uint8_t command);
void ssd1306_display(void);
void display_date_time(int8_t day, int8_t month, int16_t year, int8_t hour, int8_t minute, int8_t second);
void display_lux_val(uint16_t val);
void display_sunrise_time(int8_t sunRiseTime_hour, int8_t sunRiseTime_minute);
void display_sunset_time(int8_t sunSetTime_hour, int8_t sunSetTime_minute);

extern int16_t lux_on_limit;
extern int16_t lux_off_limit;
extern int16_t lux;
extern uint8_t hours_after_sunset;
extern uint8_t minutes_after_sunset;
extern uint8_t hours_to_sunrise;
extern uint8_t minutes_to_sunrise;

enum oled_state {start, lx_lt_blink, sunset_hour, sunset_minute, sunrise_hour, sunrise_minute, lx_gt_blink, settings, none, stop, relay_idle, relay_morning_on, relay_morning_off, relay_evening_on, relay_evening_off};




#endif /* OLED_FLES_H_ */