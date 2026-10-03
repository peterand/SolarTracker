/*
 * oled_fles.h
 *
 * Created: 06/10/2022 20:56:10
 *  Author: Peter
 */ 


#ifndef OLED_FLES_H_
#define OLED_FLES_H_

void ssd1306_init(void);
void ssd1306_command(uint8_t command);
void ssd1306_display(void);
void display_date_time(int8_t day, int8_t month, int16_t year, int8_t hour, int8_t minute, int8_t second);
void display_lux_val(uint16_t val);
void display_sunrise_time(int8_t sunRiseTime_hour, int8_t sunRiseTime_minute);
void display_sunset_time(int8_t sunSetTime_hour, int8_t sunSetTime_minute);
void calcSunRiseSunSet(uint8_t * sunRiseTime_hour, uint8_t * sunRiseTime_minute, uint8_t * sunSetTime_hour, uint8_t * sunSetTime_minute, int res_nLatitudeDegrees, int res_nLongitudeDegrees, uint8_t time_zone, float JulianDay);

int16_t lux_on_limit;
int16_t lux_off_limit;
int16_t lux;
uint8_t hours_after_sunset;
uint8_t minutes_after_sunset;
uint8_t hours_to_sunrise;
uint8_t minutes_to_sunrise;

uint8_t sunRiseTime_hour;
uint8_t sunRiseTime_minute;
uint8_t sunSetTime_hour;
uint8_t sunSetTime_minute;
//uint8_t solarNoon_hour;
//uint8_t solarNoon_minute;
uint16_t iSunRiseTime;
uint16_t iSunRiseTime_tomorrow;
uint16_t iSunSetTime;
uint16_t iSolarNoon;
//uint16_t gpsUtcNow;
uint16_t syncUtcNow;
uint16_t iGmtSecondNow;

enum uint8_t {start, lx_lt_blink, sunset_hour, sunset_minute, sunrise_hour, sunrise_minute, lx_gt_blink, settings, none, stop, relay_idle, relay_morning_on, relay_morning_off, relay_evening_on, relay_evening_off};




#endif /* OLED_FLES_H_ */