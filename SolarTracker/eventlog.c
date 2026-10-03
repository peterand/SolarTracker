/*
 * eventlog.c
 *
 * EEPROM layout, at fixed addresses so that no .eeprom section ends up in the .eep file
 * (programming that file must not touch the log):
 *   EE_MAGIC  0xA5 once the log has been initialised
 *   EE_HEAD   next slot to write
 *   EE_LOG    EVENTLOG_SLOTS records of 4 bytes, 0xFF = empty
 */

#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include "eventlog.h"

#define EE_MAGIC	((uint8_t *)0x300)
#define EE_HEAD		((uint8_t *)0x301)
#define EE_LOG		((uint8_t *)0x302)
#define LOG_MAGIC	0xA5

static const char names[] PROGMEM = "???RSTSTLWLMNSSNSE";	/* 3 letters per ev_code */

void eventlog_init(void)
{
	if (eeprom_read_byte(EE_MAGIC) != LOG_MAGIC)
	{
		for (uint8_t i = 0; i < EVENTLOG_SLOTS * 4; i++)
			eeprom_update_byte(EE_LOG + i, 0xFF);
		eeprom_update_byte(EE_HEAD, 0);
		eeprom_update_byte(EE_MAGIC, LOG_MAGIC);
	}
}

void eventlog_write(uint8_t code, uint8_t hour, uint8_t minute, uint8_t info)
{
	uint8_t head = eeprom_read_byte(EE_HEAD);
	if (head >= EVENTLOG_SLOTS) head = 0;
	uint8_t *rec = EE_LOG + head * 4;
	eeprom_update_byte(rec, code);
	eeprom_update_byte(rec + 1, hour);
	eeprom_update_byte(rec + 2, minute);
	eeprom_update_byte(rec + 3, info);
	eeprom_update_byte(EE_HEAD, (head + 1) % EVENTLOG_SLOTS);	/* last: a record cut short by power loss is not published */
}

static void put2(uint8_t v)
{
	putchar('0' + (v / 10) % 10);
	putchar('0' + v % 10);
}

static void put_hex(uint8_t v)
{
	for (uint8_t shift = 4; ; shift -= 4)
	{
		uint8_t n = (v >> shift) & 0x0F;
		putchar(n < 10 ? '0' + n : 'A' + n - 10);
		if (!shift) break;
	}
}

void eventlog_dump(void)
{
	uint8_t head = eeprom_read_byte(EE_HEAD);
	if (head >= EVENTLOG_SLOTS) head = 0;
	puts_P(PSTR("Event log, oldest first:"));
	for (uint8_t i = 0; i < EVENTLOG_SLOTS; i++)
	{
		const uint8_t *rec = EE_LOG + ((head + i) % EVENTLOG_SLOTS) * 4;
		uint8_t code = eeprom_read_byte(rec);
		if (code == EV_NONE || code >= EV_COUNT) continue;		/* empty (0xFF) or garbage */
		uint8_t hour = eeprom_read_byte(rec + 1);
		for (uint8_t c = 0; c < 3; c++) putchar(pgm_read_byte(&names[code * 3 + c]));
		putchar(' ');
		putchar((hour & 0x80) ? '~' : ' ');		/* ~ = clock not synced yet */
		put2(hour & 0x7F);
		putchar(':');
		put2(eeprom_read_byte(rec + 2));
		putchar(' ');
		put_hex(eeprom_read_byte(rec + 3));
		putchar('\n');
	}
}
