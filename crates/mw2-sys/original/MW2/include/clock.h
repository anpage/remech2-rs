#ifndef CLOCK_H
#define CLOCK_H

#include "transform.h"
#include "types.h"

// The functions and globals of clock.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_slopeSines[800];
	extern MechS32 g_slopeCosines[800];
	extern MechS32 g_currentClock;
	extern MechS32 g_framerateLimit;
	extern MechS32 g_timeExpansionEnabled;
	extern MechS32 g_timeCompressionEnabled;
	extern MechS32 g_realClock;
	extern MechS32 g_clockMode;
	extern MechS32 g_deltaTime;
	extern MechS32 g_ticksTimerInitialized;
	extern MechS16* g_sqrtTable;
	extern MechS32 g_sinTable[0x102];
	extern MechS16 g_sqrtTableData[0x400];
	extern MechS32 g_atanTable[0x102];

	MechS32 InitSinAtanTables(void);
	MechS32 InitSlopeTables(void);
	MechS32 InitSqrtTable(void);
	MechS32 Hypot2D(MechS32 p_x, MechS32 p_y);
	void BuildMatrixFromDirection(Matrix* p_matrix, MechS32 p_x, MechS32 p_y, MechS32 p_z);
	void NormalizeRotation(Matrix* p_matrix);
	void ScaleVectorToLength(MechS32 p_length, MechS32* p_x, MechS32* p_y, MechS32* p_z);
	void FirstClock(void);
	void NextClock(void);
	void StopTimers(void);
	MechS32 GetGameClock(void);
	MechS32 GetTicksSinceSync(void);
	void ResetSyncTicks(void);
	void ResetClocks(void);
	MechS32 GetRealClock(void);

#ifdef __cplusplus
}
#endif

#endif // CLOCK_H
