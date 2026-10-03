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
#include "defines.h"

		uint16_t day_number_1980(uint16_t yyyy, uint16_t mm, uint16_t dd);
		void date_1980(uint16_t days, uint16_t *year, uint16_t *month, uint16_t *day);
		void gprmc2int(char gprmc[], uint16_t *year, uint16_t *month, uint16_t *day);
		char *int2gprmc(uint16_t year, uint16_t month, uint16_t day);


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
			if (nmea_buffer[1] == 'G')
			{
				if(nmea_buffer[2] == 'P')
				{
					if(nmea_buffer[3] == 'R')
					{
						if (nmea_buffer[4] == 'M')
						{
							if(nmea_buffer[5] == 'C') new_nmea = true;
						}
					}
				}
			}
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
		for (int i = 0; i<20; i++)
		{
			for (int j=0; j<15; j++)
			{
//				loop_until_bit_is_set(UCSR0A, UDRE0);
//				UDR0 = tmp_words[i][j];
				tmp_words[i][j] = 0;
			}
		}

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
void parsedata() {
	int received_cks = 16*digit2dec(tmp_szChecksum[0]) + digit2dec(tmp_szChecksum[1]);
	//uart1.Send("seq: [cc:%X][words:%d][rc:%s:%d]\r\n", m_nChecksum,m_nWordIdx, tmp_szChecksum, received_cks);
	// check checksum, and return if invalid!
	//printf("m_nChecksum %d, received_cks %d\n", m_nChecksum, received_cks);

#if !IGNORE_CHECKSUM
	if (m_nChecksum != received_cks) {
//		printf("m_nChecksum %d, received_cks %d\n", m_nChecksum, received_cks);
		//m_bFlagDataReady = false;
		return;
	}

#endif	
	/* $GPRMC
	 * note: a siRF chipset will not support magnetic headers.
	 * $GPRMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,ddmmyy,x.x,a*hh
	 * ex: $GPRMC,230558.501,A,4543.8901,N,02112.7219,E,1.50 ,181.47,230213,,,A*66,
		   $GPRMC,195717    ,V,6008.4132,N,02426.6929,E,002.4,329.6 ,180118,,,N*61<\r><\n>

	 *
	 * WORDS:
	 *  1	 = UTC of position fix
	 *  2    = Data status (V=navigation receiver warning)
	 *  3    = Latitude of fix
	 *  4    = N or S
	 *  5    = Longitude of fix
	 *  6    = E or W
	 *  7    = Speed over ground in knots
	 *  8    = Track made good in degrees True, Bearing This indicates the direction that the device is currently moving in, 
	 *       from 0 to 360, measured in �azimuth�.
	 *  9    = UT date
	 *  10   = Magnetic variation degrees (Easterly var. subtracts from true course)
	 *  11   = E or W
	 *  12   = Checksum
	 */
//	if (mstrcmp(tmp_words[0], "$GPRMC") == 0) {
		// Check data status: A-ok, V-invalid
/*
		if (tmp_words[2][0] == 'V') {
			// clear data
			res_fLatitude = 0;
			res_fLongitude = 0;
			res_nUTCHour = digit2dec(tmp_words[1][0]) * 10 + digit2dec(tmp_words[1][1]);
			res_nUTCMin = digit2dec(tmp_words[1][2]) * 10 + digit2dec(tmp_words[1][3]);
			res_nUTCSec = digit2dec(tmp_words[1][4]) * 10 + digit2dec(tmp_words[1][5]);
			res_nUTCDay = digit2dec(tmp_words[9][0]) * 10 + digit2dec(tmp_words[9][1]);
			res_nUTCMonth = digit2dec(tmp_words[9][2]) * 10 + digit2dec(tmp_words[9][3]);
			res_nUTCYear = digit2dec(tmp_words[9][4]) * 10 + digit2dec(tmp_words[9][5]);

			m_bFlagDataReady = true;
			return;
		}
*/
		// parse time
		res_nUTCHour = digit2dec(tmp_words[1][0]) * 10 + digit2dec(tmp_words[1][1]);
		res_nUTCMin = digit2dec(tmp_words[1][2]) * 10 + digit2dec(tmp_words[1][3]);
		res_nUTCSec = digit2dec(tmp_words[1][4]) * 10 + digit2dec(tmp_words[1][5]);
		// parse latitude and longitude in NMEA format
		if (tmp_words[2][0] == 'A')
		{
			res_nLatitudeDegrees = digit2dec(tmp_words[3][0]) * 10 + digit2dec(tmp_words[3][1]);
			res_nLongitudeDegrees = digit2dec(tmp_words[5][1]) * 10 + digit2dec(tmp_words[5][2]);
			res_fLatitude = string2float(tmp_words[3]);
			res_fLongitude = string2float(tmp_words[5]);

//		}
/*
		else
		{ 
			res_nLatitudeDegrees = 60;
			res_nLongitudeDegrees = 24;
//			res_fLatitude = 6008.0;
//			res_fLongitude = 2426.0;
//			return;		//unders�ker om det funkar, res_... on�digt i s� fall
		}
*/
/*
		res_fLatitude = string2float(tmp_words[3]);
		res_fLongitude = string2float(tmp_words[5]);

		// get decimal format
		if (tmp_words[4][0] == 'S') res_fLatitude  *= -1.0;
		if (tmp_words[6][0] == 'W') res_fLongitude *= -1.0;
		float degrees = trunc(res_fLatitude / 100.0f);
		float minutes = res_fLatitude - (degrees * 100.0f);
		res_fLatitude = degrees + minutes / 60.0f;
		degrees = trunc(res_fLongitude / 100.0f);
		minutes = res_fLongitude - (degrees * 100.0f);
		res_fLongitude = degrees + minutes / 60.0f;
		//parse speed
		// The knot (pronounced not) is a unit of speed equal to one nautical mile (1.852 km) per hour
		res_fSpeed = string2float(tmp_words[7]);
		res_fSpeed *= 1.852; // convert to km/h
		// parse bearing
		res_fBearing = string2float(tmp_words[8]);
*/
		// parse UTC date
/**/
 uint16_t year, month, day;
 // On 2019-04-07 the receiver believes current Gregorian date to be 1999-08-22 and
 // $GPRMC shows that as "220899". This needs to be adjusted to correct date "070419"
 // Note that the year number in the NMEA string has only two digits
// char *gprmc = "060703";
 /**/
 char gprmc[7];
	gprmc[0] = tmp_words[9][0];
	gprmc[1] = tmp_words[9][1];
	gprmc[2] = tmp_words[9][2];
	gprmc[3] = tmp_words[9][3];
	gprmc[4] = tmp_words[9][4];
	gprmc[5] = tmp_words[9][5];
	gprmc[6] = '\0';
/**/
 // first, convert NMEA date to integer format (adds century to the year)
 gprmc2int(gprmc, &year, &month, &day);
// gprmc2int(&gprmc[0], &year, &month, &day);
//printf("%d:%d:%d\n",year,month,day);
 // calculate how many days there are since start of 1980
 uint16_t day_num = day_number_1980(year, month, day);
//printf("day_num1: %d\n",day_num);
 // adjust date only if it is before previous GPS week number roll-over
 // which happened 2019-04-06. That is 14341 days after start of 1980
 if (day_num <= 14341)
 day_num = day_num + 1024 * 7;
//printf("day_num2: %d\n",day_num);

 // convert the day number back to integer year, month and day
 date_1980(day_num, &year, &month, &day);
// date_1980(day_num, &res_nUTCYear, &res_nUTCMonth, &res_nUTCDay);
//printf("date_1980 %d:%d:%d\n",year,month,day);

 // convert the year, month and day to NMEA string format (drops century from the year)
// printf("%d-%d-%d\n",year,month,day);
 char *adjusted = int2gprmc(year, month, day);
// printf("$GPRMC date %s adjusted to %s\n", gprmc, adjusted);		

		tmp_words[9][0] = adjusted[0];
		tmp_words[9][1] = adjusted[1];
		tmp_words[9][2] = adjusted[2];
		tmp_words[9][3] = adjusted[3];
		tmp_words[9][4] = adjusted[4];
		tmp_words[9][5] = adjusted[5];

		res_nUTCDay = digit2dec(tmp_words[9][0]) * 10 + digit2dec(tmp_words[9][1]);
		res_nUTCMonth = digit2dec(tmp_words[9][2]) * 10 + digit2dec(tmp_words[9][3]);
		res_nUTCYear = digit2dec(tmp_words[9][4]) * 10 + digit2dec(tmp_words[9][5]);

/*		res_nUTCDay = 19;
		res_nUTCMonth = 1;
		res_nUTCYear = 18;
*/		
		// data ready
		m_bFlagDataReady = true;
	}		
}
/*
 * returns base-16 value of chars '0'-'9' and 'A'-'F';
 * does not trap invalid chars!
 */	
int digit2dec(char digit) {
	if ((int)(digit) >= 65) 
		return (int)(digit) - 55;
	else 
		return (int)(digit) - 48;
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

int mstrcmp(const char *s1, const char *s2)
{
	while((*s1 && *s2) && (*s1 == *s2))
	s1++,s2++;
	return *s1 - *s2;
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

int getSatellites() {
	return res_nSatellitesUsed;
}

float  getAltitude() {
	return res_fAltitude;
}

float getSpeed() {
	return res_fSpeed;
}

float getBearing() {
	return res_fBearing;
}

// known day_of_year for each month:
// Major index 0 is for non-leap years, and 1 is for leap years
// Minor index is for month number 1 .. 12, 0 at index 0 is number of days before January
static const uint16_t month_days[2][13] = {
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
	{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};
// Count the days since start of 1980
// Counts year * 356 days + leap days + month lengths + days in month
// The leap days counting needs the "+ 1" because GPS year 0 (i.e. 1980) was a leap year
uint16_t day_number_1980(uint16_t year, uint16_t month, uint16_t day)
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
void date_1980(uint16_t day_number, uint16_t *year, uint16_t *month, uint16_t *day)
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
// Convert NMEA $GPRMC date string to integer components
void gprmc2int(char gprmc[], uint16_t *year, uint16_t *month, uint16_t *day)
{
	*day = 10 * (gprmc[0] - '0') + (gprmc[1] - '0');
	*month = 10 * (gprmc[2] - '0') + (gprmc[3] - '0');
	*year = 10 * (gprmc[4] - '0') + (gprmc[5] - '0');
//	assert(*year >= 0 && *year <= 99 && *month >= 1 && *month <= 12	&& *day >= 1 && *day <= 31);
	// NMEA $GPRMC year number has only 2 digits
	if (*year > 79)
	*year = *year + 1900;
	else
	*year = *year + 2000;
}
// Convert integer date components to NMEA $GPRMC date string
char *int2gprmc(uint16_t year, uint16_t month, uint16_t day)
{
//	assert(year >= 1980 && year <= 2079 && month >= 1 && month <= 12 && day >= 1 && day <= 31);
	year = year % 100; // use only decades and years, drop centuries
	static char gprmc[7];
	gprmc[0] = '0' + (day / 10);
	gprmc[1] = '0' + (day % 10);
	gprmc[2] = '0' + (month / 10);
	gprmc[3] = '0' + (month % 10);
	gprmc[4] = '0' + (year / 10);
	gprmc[5] = '0' + (year % 10);
	gprmc[6] = '\0';
	return gprmc;
}
