/*
 * eventlog.h
 *
 * Small ring-buffer event log in EEPROM. It survives the manual CPU reset, so after finding the
 * tracker at an end stop the log shows what happened and when. It is printed on the UART at boot.
 *
 * Record (4 bytes): code, hour (UTC, bit 7 set = clock not yet synced to GPS), minute, info
 *   RST  CPU reset            info = MCUSR (1 power-on, 2 external, 4 brown-out, 8 watchdog;
 *                             0 = the bootloader had already cleared it)
 *   STL  motor stall          info = track_status when the stall timeout expired
 *   WLM  west limit reached   info = track_status; the motor was not allowed to drive further west
 *   NSS  south sensor pulse while the motor was stopped (ignored)
 *   NSE  end stop pulse while the motor was not driving east (ignored)
 * track_status: 0 g_idle, 1 go_west, 2 go_east, 3 end_stop, 4 pointing_south, 5 prep_tracking,
 *               6 tracking, 9 stalled
 * Repeats of the same code within LOG_LOCKOUT_S seconds are dropped, so noise cannot flood the log.
 */

#ifndef EVENTLOG_H_
#define EVENTLOG_H_

#include <stdint.h>

#define EVENTLOG_SLOTS 32

enum ev_code {EV_NONE, EV_RESET, EV_STALL, EV_WEST_LIMIT, EV_NOISE_SOUTH, EV_NOISE_ENDSTOP, EV_COUNT};

void eventlog_init(void);
void eventlog_write(uint8_t code, uint8_t hour, uint8_t minute, uint8_t info);
void eventlog_dump(void);	/* prints the log, oldest first, to stdout */

#endif /* EVENTLOG_H_ */
