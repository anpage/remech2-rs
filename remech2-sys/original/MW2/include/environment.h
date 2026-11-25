#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "types.h"

typedef struct TimeOfDayPhase {
	int32_t m_palette;  // 0x00
	int32_t m_duration; // 0x04
} TimeOfDayPhase;

// The functions and globals of environment.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_infraredOn;

	extern int32_t g_soundDelayPerUnit;
	extern int32_t g_gravity;
	extern int32_t g_daysPerYear;
	extern int32_t g_dayOfYear;
	extern int32_t g_secondsPerDay;
	extern int32_t g_timeOfDayPhase;
	extern int32_t g_gravityScale;
	extern TimeOfDayPhase g_timeOfDayPhases[4];
	extern int32_t g_timeOfDay;
	extern int32_t g_startTimeOfDay;
	extern int32_t g_timeOfDayFrames;
	extern int32_t g_timeOfDayStarts[4];
	extern int32_t g_timeOfDayEnabled;
	extern int32_t g_nextTimeOfDayUpdate;

	void FirstEnvironment(void);
	void UpdateTimeOfDay(void);
	void FadeToTimeOfDayPhase(int32_t p_index);
	int32_t IsInfraredOn(void);
	void SetInfrared(int32_t p_state);
	void QuadrupleMechCooling(void);
	void QuarterMechCooling(void);

#ifdef __cplusplus
}
#endif

#endif // ENVIRONMENT_H
