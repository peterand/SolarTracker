/*
 * nmea.h
 *
 * Created: 25/08/2022 10:10:19
 *  Author: Peter
 */ 


#ifndef NMEA_H_
#define NMEA_H_
/*
    File:       nmea.h
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
 #include <stdbool.h>
 #include <stdint.h>
		
extern char nmea_buffer[256];
extern volatile bool new_nmea;

extern bool		m_bFlagRead,					// flag used by the parser, when a valid sentence has begun
				m_bFlagDataReady;				// valid GPS fix and data available, user can call reader functions
extern char		tmp_words[20][15],				//	hold parsed words for one given NMEA sentence
				tmp_szChecksum[15];				//	hold the received checksum for one given NMEA sentence

// will be set to true for characters between $ and * only
extern bool		m_bFlagComputedCks;				// used to compute checksum and indicate valid checksum interval (between $ and * in a given sentence)
extern int		m_nChecksum;					// numeric checksum, computed for a given sentence
extern bool		m_bFlagReceivedCks;				// after getting  * we start cuttings the received checksum
extern int		index_received_checksum;		// used to parse received checksum

// word cutting variables
extern int		m_nWordIdx,						// the current word in a sentence
				m_nPrevIdx,						// last character index where we did a cut
				m_nNowIdx;						// current character index

// globals to store parser results
extern float	res_fLongitude;					// signed decimal degrees, east positive
extern float	res_fLatitude;					// signed decimal degrees, north positive
extern unsigned char	res_nUTCHour, res_nUTCMin, res_nUTCSec,		// GPRMC
				res_nUTCDay, res_nUTCMonth, res_nUTCYear;			// GPRMC

// the parser, currently handling GPRMC
void			parsedata(void);
// aux functions
int				digit2dec(char hexdigit);
float			string2float(char* s);

		/*
		 * The serial data is assembled on the fly, without using any redundant buffers. 
		 * When a sentence is complete (one that starts with $, ending in EOL), all processing is done on 
		 * this temporary buffer that we've built: checksum computation, extracting sentence "words" (the CSV values), 
		 * and so on.
		 * When a new sentence is fully assembled using the fusedata function, the code calls parsedata. 
		 * This function in turn, splits the sentences and interprets the data. Here is part of the parser function, 
		 * handling both the $GPRMC NMEA sentence:
		 */
		int				fusedata(char c);
		

		// READER functions: retrieving results, call isdataready() first
		bool			isdataready();
		int				getHour();
		int				getMinute();
		int				getSecond();
		int				getDay();
		int				getMonth();
		int				getYear();
		float			getLatitude();
		float			getLongitude();

#endif /* NMEA_H_ */