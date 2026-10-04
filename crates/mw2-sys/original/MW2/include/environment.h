#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "decomp.h"
#include "types.h"

// The functions and globals of environment.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_infraredOn;

	extern MechS32 g_soundDelayPerUnit;
	extern MechS32 g_gravity;
	extern MechS32 g_unk0x100ba608;
	extern MechS32 g_daysPerYear;
	extern MechS32 g_dayOfYear;
	extern MechS32 g_secondsPerDay;
	extern MechS32 g_timeOfDayPhase;
	extern MechS32 g_gravityScale;

	void FirstEnvironment(void);
	void UpdateTimeOfDay(void);
	void FadeToTimeOfDayPhase(MechS32 p_index);
	MechS32 IsInfraredOn(undefined4 p_unk0x00);
	void SetInfrared(undefined4 p_unk0x00, MechS32 p_state);
	void QuadrupleMechCooling(void);
	void QuarterMechCooling(void);

#ifdef __cplusplus
}
#endif

#endif // ENVIRONMENT_H
