/*
 * SolarTracker.c
 *
 * Created: 25/08/2022 10:07:06
 * Author : Peter
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
#include <util/eu_dst.h>
#include <inttypes.h>
#include <stdbool.h>
#include "oled_files.h"

int uart_putchar(char c, FILE *unused);

void ioinit(void);
void timer0_init(void);
void init_devices(void);

enum gear {g_idle, go_west, go_east, end_stop, pointing_south, prep_tracking, tracking, pointing_south_tracking, test};
const char *status_names[] = {"g_idle", "go_west", "go_east", "end_stop", "pointing_south", "prep_tracking", "tracking", "south_tracking", "test"};
volatile uint8_t track_status = g_idle;
volatile uint16_t rev_count = 0;
uint8_t test_minute = 0, test_hour = 0;

volatile uint8_t synced = 0;
volatile uint8_t time_s = 0;
volatile uint8_t time_m = 0;
volatile uint8_t time_h = 0;

int gYear, gMonth, gDay;
bool summer_time;

void tasker_init(void);
void task_dispatch(void);
void prio1_task(void);
void prio2_task(void);
void prio3_task(void);
void prio4_task(void);
void prio5_task(void);
void prio6_task(void);
void prio7_task(void);
void prio8_task(void);

typedef void(*f_ptr_t)(void);

f_ptr_t const tasks[] PROGMEM = {
	(f_ptr_t) prio1_task,
	(f_ptr_t) prio2_task,
	(f_ptr_t) prio3_task,
	(f_ptr_t) prio4_task,
	(f_ptr_t) prio5_task,
	(f_ptr_t) prio6_task,
	(f_ptr_t) prio7_task,
	(f_ptr_t) prio8_task
};

#define NUM_TASKS (sizeof(tasks)/sizeof(tasks[0]))
volatile uint8_t task_flag = 0; /* if non-zero, a tick has elapsed */
uint8_t task_timer[NUM_TASKS];


FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);

static inline void
stop_rotation (void)
{
	OCR1A = 0;
	OCR1B = 0;
}

static inline void
rotate_cw (void)	/*go_west*/
{
	OCR1B = 0;
	OCR1A = TROTTLE;
}

static inline void
rotate_ccw (void)	/*go_east*/
{
	OCR1A = 0;
	OCR1B = TROTTLE;
}

ISR( TIMER0_COMPA_vect )
{
	static uint8_t tasker_ticks = 0;
	if (++tasker_ticks == 10)
	{
		task_flag = 1;			/* scan tasks every 10 ms */
		tasker_ticks = 0;
	}
}

ISR( TIMER2_COMPA_vect )
{
	static uint8_t ticks;
	if (++ticks > 249)	
	{
		time_s++;
		if (time_s > 59)
		{
			time_s = 0;
			time_m++;
		}
		if (time_m > 59)
		{
			time_m = 0;
			time_h++;
		}
		if (time_h > 23)
		{
			time_h = 0;
		}
		ticks = 0;
	}
}

ISR(INT1_vect)
{
//	OCR1A = 0;
//	OCR1B = 0;
	stop_rotation();
	track_status = pointing_south;
}

ISR(INT0_vect)
{
//	OCR1A = 0;
//	OCR1B = 0;
	stop_rotation();
	rev_count++;
}

ISR(PCINT2_vect)
{
	if ((PIND & 0x10) == 0)
	{
//		OCR1A = 0;
//		OCR1B = 0;
		stop_rotation();
		track_status = end_stop;	
	}
}

void init_devices(void)
{

	DDRB |= _BV(PB0);
	PORTB |= _BV(PB0);	/* GPS FET */

	DDRD |= _BV(PD6);
	PORTD &=~_BV(PD6);  /* Dusk relay off */
	
	PORTD |= _BV(PD2);	// INT0 PULLUP
	PORTD |= _BV(PD3);	// INT1 PULLUP
	PORTD |= _BV(PD4);	// PCIN20 PULLUP END STOP

	DDRB |= _BV(PB1);	//OC1A Motor forward
	DDRB |= _BV(PB2);	//OC1B Motor reverse 

//	OCR1A = 0;
//	OCR1B = 0;
	stop_rotation();
	TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11);	//PWM, Phase Correct, 9-bit
														//Clear OC1A/OC1B on Compare Match when upcounting.
														//Set OC1A/OC1B on Compare Match when  downcounting.
	TCCR1B = _BV(CS10);									//clkIO/1 (No prescaling) -> fPWM = 15655
	EICRA = /*_BV(ISC00) | */_BV(ISC01) /*| _BV(ISC10)*/ | _BV(ISC11);
	EIMSK = _BV(INT0) | _BV(INT1);
	PCICR = _BV( PCIE2);
	PCMSK2 = _BV(PCINT20);
}

void ioinit(void)
{
	#if F_CPU <= 1000000UL
		UCSRA = _BV(U2X);
//		UBRR = (F_CPU / (8 * 9600UL)) - 1;
		UBRR = (F_CPU / (8 * 4800UL)) - 1;
	#else
//		UBRR = (F_CPU / (16 * 9600UL)) - 1;
		UBRR = (F_CPU / (16 * 4800UL)) - 1;
	#endif
	UCSRB |= _BV(TXEN0);		/* tx enable */
	UCSRB |= _BV(RXEN0);		/* rx enable */
	UCSR0B |= _BV(RXCIE0);
}

int uart_putchar(char c, FILE *unused)
{
	if (c == '\n')
	uart_putchar('\r', 0);
	loop_until_bit_is_set(UCSRA, UDRE0);
	UDR0 = c;
	return 0;
}

void USART_vSendByte(uint8_t u8Data)
{
	// Wait if a byte is being transmitted
	while((UCSR0A&(1<<UDRE0)) == 0);
	{
		// Transmit data
		UDR0 = u8Data;
	}
}

void USART_vSendStringWithNewLine(const char *str)
{
	while(*str)
	{
		USART_vSendByte(*str);
		str++;
	}
	USART_vSendByte(0x0A);
}
 
void timer0_init(void)
{
	TCCR0A = _BV(WGM01);   /* CTC mode */
//	OCR0A = 125;           /* 1 ms @ 8 MHz and prescaler = 64 */
	OCR0A = 249;           /* 1 ms @ 16 MHz and prescaler = 64 */
	TIMSK0 = _BV(OCIE0A);	/* Enable Timer/Counter0 Compare Match A interrupt */
	TCCR0B = _BV(CS00) | _BV(CS01); /* precaler 64, start timer */
	
	/* experimental attempt to spread the (initial dispatch of) tasks */
	uint8_t prime_numbers[]={2, 3, 5, 7, 11, 13, 17, 19};
	for(int i = 0; i< NUM_TASKS; i++)
	{
		task_timer[i] = prime_numbers[i & 0x07];
	}
}
void timer2_init(void)
{
	TCCR2A = _BV(WGM21);   /* CTC mode */
	OCR2A = 250;
	TIMSK2 = _BV(OCIE2A);	/* Enable Timer/Counter2 Compare Match A interrupt */
	TCCR2B = _BV(CS21) | _BV(CS22); /* prescaler 256, start timer */
}

void startGPS(void)
{
	PORTB |= _BV(PB0);
}

void stopGPS(void)
{
	PORTB &=~_BV(PB0);
}

void duskRelayOn(void)
{
	PORTD |= _BV(PD6);
}

void duskRelayOff(void)
{
	PORTD &=~_BV(PD6);
}

void task_dispatch(void)
{
	f_ptr_t fptr;
	for (uint8_t task = 0; task < NUM_TASKS; task++)
	{
		if (task_timer[task])
		{
			if (!--task_timer[task])
			{
				fptr = (f_ptr_t)pgm_read_word(&tasks[task]);
				fptr();
			}
		}
	}
}

int main(void)
{
	new_nmea = false;
	ioinit();  
	init_devices();
	stdout = &mystdout;
	printf("Preparing..\n");  
	timer0_init();
	timer2_init();
	startGPS();
	sei();
	printf("..To Start..\n");
	
	while(1)
	{
		if (task_flag)
		{
			task_flag = 0;
			task_dispatch();
		}
	}
}

int nmea0183_checksum(char *nmea_data)
{
	int crc = 0;
	int i;

	// the first $ sign and the last two bytes of original CRC + the * sign
	size_t len = strlen(nmea_data);
	for (i = 1; len >= 3 && i < (int)(len - 3); i ++) {
		crc ^= nmea_data[i];
	}

	return crc;
}

void prio1_task(void)
{

#if !TEST_GMT
	struct tm *stmPtr, stmData;
	stmPtr = &stmData;
	time_t t_utc_now=0;

	if (new_nmea)
	{
		for (int i=0; i<256; i++)
		{
			fusedata(nmea_buffer[i]);
			if (nmea_buffer[i] == '\n') break;
		}

		for (int i=0; i<256; i++) nmea_buffer[i] = 0;

		if (isdataready())
		{			
			stmPtr->tm_year = getYear()+100;
			stmPtr->tm_mon = getMonth()-1;
			stmPtr->tm_mday = getDay(); 

			stmPtr->tm_hour = getHour();
			stmPtr->tm_min = getMinute();
			stmPtr->tm_sec = getSecond();
			
			t_utc_now = mk_gmtime (stmPtr);
			
			if (eu_dst(&t_utc_now,0) > 0) summer_time = true;
//			if ((getMonth() > APRIL) && (getMonth() < NOVEMBER)) summer_time = true; 
			else summer_time = false;
			
//			gDay = stmPtr -> tm_mday;
//			gMonth = stmPtr -> tm_mon;
//			gYear = stmPtr -> tm_year; /* years since 1900 */

//			printf("t_gmt %lu\n", (unsigned long)t_gmt_now);
			
			int32_t lat = res_nLatitudeDegrees;
			int32_t lon = res_nLongitudeDegrees;
			set_position(lat * ONE_DEGREE, lon * ONE_DEGREE);

			float julian_day = (float)t_utc_now/ONE_DAY;
			
//			printf("julian_day %.2f\n", julian_day);
			
			calcSunRiseSunSet(&sunRiseTime_hour, &sunRiseTime_minute, &sunSetTime_hour, &sunSetTime_minute, res_nLatitudeDegrees, res_nLongitudeDegrees, 0, julian_day+1);
			iSunRiseTime_tomorrow = iSunRiseTime;
			calcSunRiseSunSet(&sunRiseTime_hour, &sunRiseTime_minute, &sunSetTime_hour, &sunSetTime_minute, res_nLatitudeDegrees, res_nLongitudeDegrees, 0, julian_day);

			cli();
			time_s = getSecond();
			time_m = getMinute();
			time_h = getHour();
			syncUtcNow = time_h * 60 + time_m;
			sei();
			synced = 1;
			stopGPS();
		}
		new_nmea = false;
	}
	else if (synced == 1)
	{
		cli();
		syncUtcNow = time_h * 60 + time_m;
		if ((syncUtcNow == 1) && (time_s == 1)) startGPS();			// Start GPS 1 minute past UTC midnight
		if ((syncUtcNow % 8 == 0) && (time_s == 30)) time_s = 31;	// Adjust free running timekeeper
		sei();
	}
	
	task_timer[PRIO_1] = 79;
#endif

#if TEST_GMT
	static uint16_t gmt = 300;
	gmt++;
	if (gmt == 1441) gmt = 0;
//	gpsUtcNow = gmt;
	iSunRiseTime = 310;
	iSunSetTime = 950;
	iSolarNoon = 615;
//	printf("iGmtNow %d\n", iGmtNow)
	task_timer[PRIO_1] = 5;
#endif
}

void prio2_task(void)
{
	task_timer[PRIO_2] = 0;
}

void prio3_task(void)
{
	uint16_t go_minutes = 0;
	static uint16_t iGo_revs;
	enum uint8_t {stay, east, west, step};
	static uint8_t go_dir = stay;
	static uint16_t iPrev_Utc = 0;
	static uint8_t old_trackstatus;

	if (old_trackstatus != track_status)
	{
//		printf("Status: %s, GMT %d, Rise %d, Noon %d, Set %d, GoRevs %d, RevCount %d, Dir %d\n", status_names[track_status], syncUtcNow, iSunRiseTime, iSolarNoon, iSunSetTime, iGo_revs, rev_count, go_dir);
	}
//	if ((iGmtNow % 100) == 0) printf("%d\n", iGmtNow);
	switch (track_status)
	{
		case g_idle:
		old_trackstatus = track_status;
		track_status = go_east;
		break;
		
		case go_west:
		old_trackstatus = track_status;
//		OCR1B = 0;
//		OCR1A = TROTTLE;
		rotate_cw();
		break;				//Next end_stop or pointing_south from interrupts
				
		case end_stop:
		old_trackstatus = track_status;
		track_status = go_west;
		break;

		case go_east:
		old_trackstatus = track_status;
//		OCR1A = 0;
//		OCR1B = TROTTLE;
		rotate_ccw();
		break;
		
		case pointing_south:
		old_trackstatus = track_status;

#if !TEST_GMT
		if ((synced == 1) && (summer_time == true))
#endif
		{
//			printf("SunRise %d Noon %d SunSet %d, Now %d\n", iSunRiseTime, iSolarNoon, iSunSetTime, iGmtNow);
			go_minutes = 0;
			if((syncUtcNow < iSunRiseTime) || (syncUtcNow > iSunSetTime))  //0..sunrise || sunset..24 
			{
				go_minutes = iSolarNoon - iSunRiseTime_tomorrow;
				go_dir = east;
			}

			else if (syncUtcNow < iSolarNoon)
			{
				go_minutes = iSolarNoon - syncUtcNow;		//Time in minutes. 288 revolutions -> 360 degrees = 21600". Earth rotates 15 deg/h = 15"/min.
				go_dir = east;
			}
			else if (syncUtcNow > iSolarNoon)
			{
				go_minutes = syncUtcNow - iSolarNoon;
				go_dir = west;
			}
			iGo_revs = go_minutes/5;			
			rev_count = 0;
			if (iGo_revs) track_status = prep_tracking;
		}
		break;

		case prep_tracking:
		old_trackstatus = track_status;
		if (rev_count >= iGo_revs)
		{
//			OCR1A = 0;
//			OCR1B = 0;
			iPrev_Utc = syncUtcNow;
			if((syncUtcNow >= iSunRiseTime) && (syncUtcNow <= iSunSetTime))
			{ 
				track_status = tracking;
			}
		}

		else if (go_dir == east)  //g_gear = g_reverse
		{
//			OCR1A = 0;
//			OCR1B = TROTTLE;
			rotate_ccw();
		}
		else if (go_dir == west)  //g_gear = g_forw
		{
//			OCR1B = 0;
//			OCR1A = TROTTLE;
			rotate_cw();
		}
		break;
		
		case tracking:
//		printf("GmtNow: %d Go_start: %d Go_stop: %d RevCount: %d\n", iGmtNow, go_minutes_start, go_minutes_stop, rev_count);
		old_trackstatus = track_status;
		
		if ((iPrev_Utc != syncUtcNow) && ((syncUtcNow % 5) == 0))
		{
			iPrev_Utc = syncUtcNow;
//			OCR1B = 0;
//			OCR1A = TROTTLE;
			rotate_cw();
		}
		else if (syncUtcNow > iSunSetTime) 
		{
//			OCR1A = 0;
//			OCR1B = TROTTLE;
			track_status = go_east;
			rotate_ccw();
		}
		break; 

		default:
		break;
	}	

#if TEST_GMT
	task_timer[PRIO_3] = 2;
#else
	task_timer[PRIO_3] = 11;
#endif
}

void prio4_task(void)
{
task_timer[PRIO_4] = 0;
}

void prio5_task(void)
{
	
	task_timer[PRIO_5] = 0;
}

void prio6_task(void)
{
	if (synced)
	{
		if((syncUtcNow > (iSunSetTime - DUSK_RELAY_ON_OFFSET)) || (syncUtcNow < (iSunRiseTime + DUSK_RELAY_ON_OFFSET)) ) duskRelayOn();
		else duskRelayOff();
	}

	task_timer[PRIO_6] = 255; 
}

void prio7_task(void)
{
//	int m_nChecksum;
//	char longstring[96]; 
//	char warning[] = {'V'};	
//	if (isdataready()) 
//	{
//		warning[0] = 'A';
//	}

//	sprintf(longstring,"$GPRMC,%2.2u%2.2u%2.2u,A,6010.97420,N,02251.50727,E,0.816,,151022,,,A*74", time_h, time_m, time_s);
//	sprintf(longstring,"$GPRMC,%2.2u%2.2u%2.2u,%c,%f,N,%f,E,0.816,,%2.2u%2.2u%2.2u,,,A*74", time_h, time_m, time_s, *warning, getLatitude(), getLongitude(), getDay(), getMonth(), getYear());
//	m_nChecksum = nmea0183_checksum(longstring);

//	sprintf(longstring,"$GPRMC,%2.2u%2.2u%2.2u,A,6010.97420,N,02251.50727,E,0.816,,151022,,,A*%2X", time_h, time_m, time_s, m_nChecksum);
//	sprintf(longstring,"$GPRMC,%2.2u%2.2u%2.2u,%c,%f,N,%f,E,0.816,,%2.2u%2.2u%2.2u,,,A*%2X", time_h, time_m, time_s, *warning, getLatitude(), getLongitude(), getDay(), getMonth(), getYear(), m_nChecksum);
//	USART_vSendStringWithNewLine(longstring);

//    printf("Status: %s, %2.2u:%2.2u:%2.2u, Rise %2.2u:%2.2u, Rise_tomorrow %2.2u:%2.2u, Noon %2.2u:%2.2u, Set %2.2u:%2.2u", status_names[track_status], time_h, time_m, time_s, iSunRiseTime/60, iSunRiseTime%60, iSunRiseTime_tomorrow/60, iSunRiseTime_tomorrow%60, iSolarNoon/60, iSolarNoon%60, iSunSetTime/60, iSunSetTime%60);
	
	task_timer[PRIO_7] = 100; 
}

void prio8_task(void)
{
	static uint8_t counter = 4;
	if (!counter--)
	{
			duskRelayOff();			
			task_timer[PRIO_8] = 0;
	} 
	else
	{
        if (counter&0x01) duskRelayOff();
		else duskRelayOn();		
		task_timer[PRIO_8] = 100;
	}
}
