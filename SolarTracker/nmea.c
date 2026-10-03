/*
 * nmea.c
 *
 * Created: 25/08/2022 10:09:18
 *  Author: Peter
 */ 
/*
    File:       nmea.cpp
    Version:    0.1.0
    Date:       Feb. 23, 2013
	License:	GPL v2
    
	NMEA GPS content parser
    
    ****************************************************************************
    Copyright (C) 2013 Radu Motisan  <radu.motisan@gmail.com>
	
	http://www.pocketmagic.net

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    ****************************************************************************
 */

#include "nmea.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <avr/interrupt.h>
#include <string.h>
#include "defines.h"

char nmea_buffer[256];
volatile bool new_nmea;

bool	m_bFlagRead, m_bFlagDataReady;
char	tmp_words[20][15], tmp_szChecksum[15];
bool	m_bFlagComputedCks;
int		m_nChecksum;
bool	m_bFlagReceivedCks;
int		index_received_checksum;
int		m_nWordIdx, m_nPrevIdx, m_nNowIdx;
float	res_fLongitude, res_fLatitude;
unsigned char res_nUTCHour, res_nUTCMin, res_nUTCSec,
		res_nUTCDay, res_nUTCMonth, res_nUTCYear;

static uint16_t day_number_1980(uint16_t yyyy, uint16_t mm, uint16_t dd);
static void date_1980(uint16_t days, uint16_t *year, uint16_t *month, uint16_t *day);

/* Buffers $GPRMC / $GNRMC (any talker ID); other sentences are dropped, as is everything while one waits to be parsed */
ISR(USART_RX_vect)
{
	static uint8_t buff_pos = 0;
	char c = UDR0; //Read the value out of the UART buffer
	if (!new_nmea)
	{
		if (c == '$')
		{
			buff_pos = 0;
		}

		nmea_buffer[buff_pos++] = c;
		if (c == '\n')
		{
			if (nmea_buffer[1] == 'G' && nmea_buffer[3] == 'R' && nmea_buffer[4] == 'M' && nmea_buffer[5] == 'C')
				new_nmea = true;
		}
	}
}


/*
 * The serial data is assembled on the fly, without using any redundant buffers. 
 * When a sentence is complete (one that starts with $, ending in EOL), all processing is done on 
 * this temporary buffer that we've built: checksum computation, extracting sentence "words" (the CSV values), 
 * and so on.
 * When a new sentence is fully assembled using the fusedata function, the code calls parsedata. 
 * This function in turn, splits the sentences and interprets the data. Here is part of the parser function, 
 * handling both the $GPRMC NMEA sentence:
 */
int fusedata(char c) {
	
	if (c == '$') {
		m_bFlagRead = true;
		m_bFlagComputedCks = false;
		m_nChecksum = 0;
		// after getting  * we start cuttings the received m_nChecksum
		m_bFlagReceivedCks = false;
		m_bFlagDataReady = false;
		index_received_checksum = 0;
		// word cutting variables
		m_nWordIdx = 0; m_nPrevIdx = 0; m_nNowIdx = 0;
		memset(tmp_words, 0, sizeof(tmp_words));
	}
	
	if (m_bFlagRead) {
		// check ending
		if (c == '\r' || c== '\n') {
			// catch last ending item too
			if (m_nWordIdx < 20 && (m_nNowIdx - m_nPrevIdx) < 15)
				tmp_words[m_nWordIdx][m_nNowIdx - m_nPrevIdx] = 0;
			m_nWordIdx++;
			// cut received m_nChecksum
			if (index_received_checksum < 15)
				tmp_szChecksum[index_received_checksum] = 0;
			// sentence complete, read done
			m_bFlagRead = false;
			// parse
			parsedata();
		} else {
			// computed m_nChecksum logic: count all chars between $ and * exclusively
			if (m_bFlagComputedCks && c == '*') m_bFlagComputedCks = false;
			if (m_bFlagComputedCks) m_nChecksum ^= c;
			if (c == '$') m_bFlagComputedCks = true;
			// received m_nChecksum
			if (m_bFlagReceivedCks && index_received_checksum < 15)  {
				tmp_szChecksum[index_received_checksum] = c;
				index_received_checksum++;
			}
			if (c == '*') m_bFlagReceivedCks = true;
			// build a word (bounds-checked: tmp_words is [20][15])
			if (m_nWordIdx < 20 && (m_nNowIdx - m_nPrevIdx) < 15)
				tmp_words[m_nWordIdx][m_nNowIdx - m_nPrevIdx] = c;
			if (c == ',') {
				if (m_nWordIdx < 20 && (m_nNowIdx - m_nPrevIdx) < 15)
					tmp_words[m_nWordIdx][m_nNowIdx - m_nPrevIdx] = 0;
				m_nWordIdx++;
				m_nPrevIdx = m_nNowIdx;
			}
			else m_nNowIdx++;
		}
	}
	return m_nWordIdx;
}


/*
 * parse internal tmp_ structures, fused by pushdata, and set the data flag when done
 */
void parsedata(void) {
#if !IGNORE_CHECKSUM
	int received_cks = 16*digit2dec(tmp_szChecksum[0]) + digit2dec(tmp_szChecksum[1]);
	// check checksum, and return if invalid!
	if (m_nChecksum != received_cks) return;
#endif
	/* $GPRMC
	 * $GPRMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,ddmmyy,x.x,a*hh
	 * ex: $GPRMC,230558.501,A,4543.8901,N,02112.7219,E,1.50 ,181.47,230213,,,A*66
	 *
	 * WORDS:
	 *  1	 = UTC of position fix
	 *  2    = Data status (A=valid, V=navigation receiver warning)
	 *  3    = Latitude of fix (ddmm.mmmm)
	 *  4    = N or S
	 *  5    = Longitude of fix (dddmm.mmmm)
	 *  6    = E or W
	 *  9    = UT date (ddmmyy)
	 */
	// parse time
	res_nUTCHour = digit2dec(tmp_words[1][0]) * 10 + digit2dec(tmp_words[1][1]);
	res_nUTCMin = digit2dec(tmp_words[1][2]) * 10 + digit2dec(tmp_words[1][3]);
	res_nUTCSec = digit2dec(tmp_words[1][4]) * 10 + digit2dec(tmp_words[1][5]);

	if (tmp_words[2][0] != 'A') return;	// no valid fix

	// parse the date first, a bogus one must not mark the data ready
	uint16_t day = digit2dec(tmp_words[9][0]) * 10 + digit2dec(tmp_words[9][1]);
	uint16_t month = digit2dec(tmp_words[9][2]) * 10 + digit2dec(tmp_words[9][3]);
	uint16_t year = digit2dec(tmp_words[9][4]) * 10 + digit2dec(tmp_words[9][5]);
	if (day < 1 || day > 31 || month < 1 || month > 12) return;
	year += (year > 79) ? 1900 : 2000;	// NMEA year number has only 2 digits

	// On 2019-04-07 the receiver believes current Gregorian date to be 1999-08-22 and
	// $GPRMC shows that as "220899". Adjust dates before the GPS week number roll-over
	// of 2019-04-06 (14341 days after start of 1980) by 1024 weeks.
	uint16_t day_num = day_number_1980(year, month, day);
	if (day_num <= 14341)
		day_num += 1024 * 7;
	date_1980(day_num, &year, &month, &day);

	res_nUTCDay = day;
	res_nUTCMonth = month;
	res_nUTCYear = year % 100;

	// position as signed decimal degrees
	res_fLatitude = digit2dec(tmp_words[3][0]) * 10 + digit2dec(tmp_words[3][1]) + string2float(&tmp_words[3][2]) / 60.0F;
	if (tmp_words[4][0] == 'S') res_fLatitude = -res_fLatitude;
	res_fLongitude = digit2dec(tmp_words[5][0]) * 100 + digit2dec(tmp_words[5][1]) * 10 + digit2dec(tmp_words[5][2]) + string2float(&tmp_words[5][3]) / 60.0F;
	if (tmp_words[6][0] == 'W') res_fLongitude = -res_fLongitude;

	// data ready
	m_bFlagDataReady = true;
}
/*
 * returns base-16 value of chars '0'-'9' and 'A'-'F';
 * does not trap invalid chars!
 */	
int digit2dec(char digit) {
	if (digit >= 'a') return digit - 'a' + 10;
	if (digit >= 'A') return digit - 'A' + 10;
	return digit - '0';
}

/* returns base-10 value of zero-terminated string
 * that contains only chars '+','-','0'-'9','.';
 * does not trap invalid strings! 
 */
float string2float(char* s) {
	long  integer_part = 0;
	float decimal_part = 0.0;
	float decimal_pivot = 0.1;
	bool isdecimal = false, isnegative = false;
	
	char c;
	while ( ( c = *s++) )  { 
		// skip special/sign chars
		if (c == '-') { isnegative = true; continue; }
		if (c == '+') continue;
		if (c == '.') { isdecimal = true; continue; }
		
		if (!isdecimal) {
			integer_part = (10 * integer_part) + (c - 48);
		}
		else {
			decimal_part += decimal_pivot * (float)(c - 48);
			decimal_pivot /= 10.0;
		}
	}
	// add integer part
	decimal_part += (float)integer_part;
	
	// check negative
	if (isnegative)  decimal_part = - decimal_part;

	return decimal_part;
}

bool isdataready() {
	return m_bFlagDataReady;
}

int getHour() {
	return res_nUTCHour;
}	
int getMinute() {
	return res_nUTCMin;
}
int getSecond() {
	return res_nUTCSec;
}
int getDay() {
	return res_nUTCDay;
}
int getMonth() {
	return res_nUTCMonth;
}
int getYear() {
	return res_nUTCYear;
}

float getLatitude() {
	return res_fLatitude;
}

float getLongitude() {
	return res_fLongitude;
}

// known day_of_year for each month:
// Major index 0 is for non-leap years, and 1 is for leap years
// Minor index is for month number 1 .. 12, 0 at index 0 is number of days before January
static const uint16_t month_days[2][13] = {
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
	{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};
// Count the days since start of 1980
// Counts year * 365 days + leap days + month lengths + days in month
// The leap days counting needs the "+ 1" because GPS year 0 (i.e. 1980) was a leap year
static uint16_t day_number_1980(uint16_t year, uint16_t month, uint16_t day)
{
	uint16_t gps_years = year - 1980;
	uint16_t leap_year = (gps_years % 4 == 0) ? 1 : 0;
	uint16_t day_of_year = month_days[leap_year][month - 1] + day;
	if (gps_years == 0)
	return day_of_year;
	return gps_years * 365 + ((gps_years - 1) / 4) + 1 + day_of_year;
}
// Convert day_number since start of 1980 to year, month, and day:
// - integer division of (day_number - 1) by 365.25 gives year number for 1980 to 2099
// - day number - (year number * 365 days + leap days) gives day of year
// The leap days needs "+ 1" because GPS year 0 (i.e. 1980) was a leap year
// - (day_of_year - 1) / 31 + 1 gives lower limit for month, but this may be one too low
// the guessed month is adjusted by checking the month lengths
// - days in month is left when the month lengths are subtracted
// - year must still be adjusted by 1980
static void date_1980(uint16_t day_number, uint16_t *year, uint16_t *month, uint16_t *day)
{
	uint16_t gps_years = ((day_number - 1) * 100UL) / 36525UL;
	uint16_t leap_year = (gps_years % 4 == 0) ? 1 : 0;
	uint16_t day_of_year = day_number;
	if (gps_years > 0) day_of_year = day_number - (gps_years * 365 + ((gps_years - 1) / 4) + 1);
	uint16_t month_of_year = (day_of_year - 1) / 31 + 1;
	if (day_of_year > month_days[leap_year][month_of_year])
	month_of_year++;
	*day = day_of_year - month_days[leap_year][month_of_year - 1];
	*month = month_of_year;
	*year = 1980 + gps_years;
}
