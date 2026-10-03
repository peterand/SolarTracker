/*
 * sun.h
 *
 * Sunrise, sunset and solar noon calculation (NOAA spreadsheet algorithm)
 */

#ifndef SUN_H_
#define SUN_H_

#include <stdint.h>

/* Results, in minutes since 00:00 UTC. Signed: sunrise can fall before UTC midnight
 * and sunset after it, and the relay/tracking maths subtracts offsets from them. */
extern int16_t iSunRiseTime;
extern int16_t iSunRiseTime_tomorrow;
extern int16_t iSunSetTime;
extern int16_t iSolarNoon;
extern int16_t syncUtcNow;		/* current UTC time of day in minutes, as of the last task_1 run */

/* latitude/longitude in signed decimal degrees (north/east positive),
 * JulianDay in days since 2000-01-01 12:00 UTC. Fills iSunRiseTime, iSunSetTime, iSolarNoon. */
void calcSunRiseSunSet(float latitude, float longitude, uint8_t time_zone, float JulianDay);

#endif /* SUN_H_ */
