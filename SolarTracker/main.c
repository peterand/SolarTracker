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
#include <util/atomic.h>
#include <math.h>
#include "nmea.h"
#include "sun.h"
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

enum gear {g_idle, go_west, go_east, end_stop, pointing_south, prep_tracking, tracking, pointing_south_tracking, test, stalled};
volatile uint8_t track_status = g_idle;
volatile uint16_t rev_count = 0;
volatile uint8_t stall_s = 0;		/* seconds the motor has run since the last sensor event */
volatile uint8_t rev_lockout = 0;	/* ms left in which further INT0 pulses are treated as contact bounce */

volatile uint8_t synced = 0;
volatile uint8_t time_s = 0;
volatile uint8_t time_m = 0;
volatile uint8_t time_h = 0;

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
	stall_s = 0;	/* every sensor ISR stops the motor, so this also restarts the stall timer */
}

/* Start the motor, but only while track_status still equals expected_status. The sensor ISRs
 * change track_status and stop the motor; checking and driving in one atomic step keeps a
 * late restart from running the motor past a sensor. The 16-bit OCR1x writes also need to be
 * atomic: they share the TEMP register with the ISRs' OCR1x writes. */
static void
drive (uint8_t expected_status, bool cw)	/* cw = go_west, !cw = go_east */
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if (track_status == expected_status)
		{
			if (cw)
			{
				OCR1B = 0;
				OCR1A = TROTTLE;
			}
			else
			{
				OCR1A = 0;
				OCR1B = TROTTLE;
			}
		}
	}
}

/* Change track_status from one state to another, unless an ISR changed it in the meantime */
static bool
status_change (uint8_t from, uint8_t to)
{
	bool changed = false;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if (track_status == from)
		{
			track_status = to;
			changed = true;
		}
	}
	return changed;
}

ISR( TIMER0_COMPA_vect )
{
	if (rev_lockout) rev_lockout--;
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

		/* Stall protection: the motor must reach a sensor (INT0/INT1/end stop) within STALL_TIMEOUT_S */
		if (OCR1A || OCR1B)
		{
			if (++stall_s >= STALL_TIMEOUT_S)
			{
				stop_rotation();
				track_status = stalled;	/* latched: nothing restarts the motor until reset or a sensor event */
			}
		}
		else stall_s = 0;
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
	if (!rev_lockout)
	{
		rev_count++;
		rev_lockout = 20;
	}
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

	/* PCINT only reports changes: if the carriage starts on the end stop, no edge will ever come */
	_delay_us(50);	/* pull-up settling */
	if ((PIND & _BV(PD4)) == 0) track_status = end_stop;
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
	// Transmit data
	UDR0 = u8Data;
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
	OCR2A = 249;           /* 16 MHz / 256 / (249 + 1) = 250 Hz, the ISR counts 250 of them per second */
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
	puts_P(PSTR("Preparing.."));
	timer0_init();
	timer2_init();
	startGPS();
	sei();
	puts_P(PSTR("..To Start.."));
	
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
		for (uint16_t i=0; i<sizeof(nmea_buffer); i++)
		{
			fusedata(nmea_buffer[i]);
			if (nmea_buffer[i] == '\n') break;
		}

		if (isdataready())
		{			
			stmPtr->tm_year = getYear()+100;
			stmPtr->tm_mon = getMonth()-1;
			stmPtr->tm_mday = getDay(); 

			stmPtr->tm_hour = getHour();
			stmPtr->tm_min = getMinute();
			stmPtr->tm_sec = getSecond();
			
			t_utc_now = mk_gmtime (stmPtr);
			
			summer_time = (eu_dst(&t_utc_now,0) > 0);
			
			set_position((int32_t)(res_fLatitude * ONE_DEGREE), (int32_t)(res_fLongitude * ONE_DEGREE));

			/* days since 2000-01-01 12:00 UTC (J2000.0); the time_t epoch is 2000-01-01 00:00 */
			float julian_day = (float)t_utc_now/ONE_DAY - 0.5F;
			
			calcSunRiseSunSet(res_fLatitude, res_fLongitude, 0, julian_day+1);
			iSunRiseTime_tomorrow = iSunRiseTime;
			calcSunRiseSunSet(res_fLatitude, res_fLongitude, 0, julian_day);

			ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
			{
				time_s = getSecond();
				time_m = getMinute();
				time_h = getHour();
				syncUtcNow = time_h * 60 + time_m;
			}
			synced = 1;
			stopGPS();
		}
		new_nmea = false;
	}
	else if (synced == 1)
	{
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			syncUtcNow = time_h * 60 + time_m;
			if ((syncUtcNow == 1) && (time_s == 1)) startGPS();			// Start GPS 1 minute past UTC midnight
		}
	}
	
	task_timer[PRIO_1] = 79;
#endif

#if TEST_GMT
	static uint16_t gmt = 300;
	gmt++;
	if (gmt == 1441) gmt = 0;
	iSunRiseTime = 310;
	iSunSetTime = 950;
	iSolarNoon = 615;
	task_timer[PRIO_1] = 5;
#endif
}

void prio2_task(void)
{
	task_timer[PRIO_2] = 0;
}

void prio3_task(void)
{
	int16_t go_minutes = 0;
	static uint16_t iGo_revs;
	enum {stay, east, west};
	static uint8_t go_dir = stay;
	static int16_t iPrev_Utc = 0;
	uint16_t revs;

	uint8_t status = track_status;		/* may be changed by the sensor ISRs at any time */
	switch (status)
	{
		case g_idle:
		status_change(g_idle, go_east);
		break;
		
		case go_west:
		drive(go_west, true);
		break;				//Next end_stop or pointing_south from interrupts
				
		case end_stop:
		status_change(end_stop, go_west);
		break;

		case go_east:
		drive(go_east, false);
		break;
		
		case pointing_south:
#if !TEST_GMT
		if ((synced == 1) && (summer_time == true))
#endif
		{
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
			iGo_revs = (go_minutes > 0) ? go_minutes/5 : 0;
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
			{
				rev_count = 0;
			}
			if (iGo_revs) status_change(pointing_south, prep_tracking);
		}
		break;

		case prep_tracking:
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			revs = rev_count;
		}
		if (revs >= iGo_revs)
		{
			iPrev_Utc = syncUtcNow;
			if((syncUtcNow >= iSunRiseTime) && (syncUtcNow <= iSunSetTime))
			{ 
				status_change(prep_tracking, tracking);
			}
		}
		else if (go_dir == east)  //g_gear = g_reverse
		{
			drive(prep_tracking, false);
		}
		else if (go_dir == west)  //g_gear = g_forw
		{
			drive(prep_tracking, true);
		}
		break;
		
		case tracking:
		if ((iPrev_Utc != syncUtcNow) && ((syncUtcNow % 5) == 0))
		{
			iPrev_Utc = syncUtcNow;
			drive(tracking, true);
		}
		else if (syncUtcNow > iSunSetTime) 
		{
			if (status_change(tracking, go_east)) drive(go_east, false);
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
		if((syncUtcNow > (iSunSetTime - DUSK_RELAY_ON_OFFSET)) || (syncUtcNow < (iSunRiseTime + DUSK_RELAY_ON_OFFSET))) duskRelayOn();	/* int16_t: sunrise before 60 min UTC must not wrap */
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
