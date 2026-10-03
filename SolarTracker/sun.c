/*
 * sun.c
 *
 * Sunrise, sunset and solar noon calculation, following the NOAA solar calculator
 * spreadsheet (column letters in the comments refer to it).
 */

#include <math.h>
#include "defines.h"
#include "sun.h"

int16_t iSunRiseTime;
int16_t iSunRiseTime_tomorrow;
int16_t iSunSetTime;
int16_t iSolarNoon;
int16_t syncUtcNow;

#define DEG2RAD (M_PI / 180)

static float wrap360(float x)
{
	while (x >= 360.0F) x -= 360.0F;
	while (x < 0.0F) x += 360.0F;
	return x;
}

void calcSunRiseSunSet(float latitude, float longitude, uint8_t time_zone, float JulianDay)
{
	float G = JulianDay / 36525.0F;		/* Julian century */

	/* Geom Mean Long Sun (deg), I2 */
	float I = wrap360(280.46646F + G * (36000.76983F + G * 0.0003032F));
	/* Geom Mean Anom Sun (deg), J2 */
	float J = wrap360(357.52911F + G * (35999.05029F - 0.0001537F * G));

	/* Sun Eq of Ctr, L2 */
	float L = sin(J * DEG2RAD) * (1.914602F - G * (0.004817F + 0.000014F * G))
	        + sin(2 * J * DEG2RAD) * (0.019993F - 0.000101F * G)
	        + sin(3 * J * DEG2RAD) * 0.000289F;

	/* Sun App Long (deg), P2 */
	float omega = (125.04F - 1934.136F * G) * DEG2RAD;
	float P = I + L - 0.00569F - 0.00478F * sin(omega);

	/* Mean Obliq Ecliptic (deg), Q2 */
	float Q = 23.0F + (26.0F + (21.448F - G * (46.815F + G * (0.00059F - G * 0.001813F))) / 60.0F) / 60.0F;
	/* Obliq Corr (deg), R2 */
	float R = Q + 0.00256F * cos(omega);

	/* Sun Declin (deg), T2 */
	float decl = asin(sin(R * DEG2RAD) * sin(P * DEG2RAD));	/* radians */

	/* var y, U2 */
	float y = tan(R * DEG2RAD / 2);
	y = y * y;

	/* Eq of Time (minutes), V2 */
	float K = ECCENT_EARTH_ORBIT;
	float sinJ = sin(J * DEG2RAD);
	float EqOfTime_minutes = (720.0F / M_PI) * (y * sin(2 * I * DEG2RAD)
	                                           - 2.0F * K * sinJ
	                                           + 4.0F * K * y * sinJ * cos(2 * I * DEG2RAD)
	                                           - 0.5F * y * y * sin(4 * I * DEG2RAD)
	                                           - 1.25F * K * K * sin(2 * J * DEG2RAD));

	/* HA Sunrise (deg), W2. Clamped so polar day/night gives 0/180 deg instead of NaN. */
	float c = cos(90.833F * DEG2RAD) / (cos(latitude * DEG2RAD) * cos(decl))
	        - tan(latitude * DEG2RAD) * tan(decl);
	if (c > 1.0F) c = 1.0F;
	if (c < -1.0F) c = -1.0F;
	float HA_Sunrise_deg = acos(c) / DEG2RAD;

	/* Solar Noon, X2 (fraction of a day) */
	float SolarNoon = (720.0F - 4.0F * longitude - EqOfTime_minutes + time_zone * 60) / 1440.0F;

	iSunRiseTime = (int16_t)((SolarNoon - HA_Sunrise_deg / 360.0F) * 1440.0F);
	iSunSetTime = (int16_t)((SolarNoon + HA_Sunrise_deg / 360.0F) * 1440.0F);
	iSolarNoon = (int16_t)(SolarNoon * 1440.0F);
}
