#ifndef CLOCK_H
#define CLOCK_H

#include "transform.h"
#include "types.h"

// The functions and globals of clock.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_slopeSines[800];
	extern int32_t g_slopeCosines[800];
	extern int32_t g_currentClock;
	extern int32_t g_framerateLimit;
	extern int32_t g_timeExpansionEnabled;
	extern int32_t g_timeCompressionEnabled;
	extern int32_t g_realClock;
	extern int32_t g_clockMode;
	extern int32_t g_deltaTime;
	extern int32_t g_ticksTimerInitialized;
	extern int16_t* g_sqrtTable;
	extern int32_t g_sinTable[0x102];
	extern int16_t g_sqrtTableData[0x400];
	extern int32_t g_atanTable[0x102];
	extern int32_t g_clockPaused;
	extern int32_t g_clockHandle;
	extern int32_t g_syncTicksHandle;
	extern int32_t g_realClockHandle;
	extern int32_t g_previousClock;
	extern int32_t g_clockModeBeforePause;

	int32_t InitSinAtanTables(void);
	int32_t InitSlopeTables(void);
	int32_t InitSqrtTable(void);
	int32_t Hypot2D(int32_t p_x, int32_t p_y);
	void BuildMatrixFromDirection(Matrix* p_matrix, int32_t p_x, int32_t p_y, int32_t p_z);
	void NormalizeRotation(Matrix* p_matrix);
	void ScaleVectorToLength(int32_t p_length, int32_t* p_x, int32_t* p_y, int32_t* p_z);
	void FirstClock(void);
	// Implemented on the Rust side (src/sim/timing.rs), around NextClockC
	void NextClock(void);
	void NextClockC(void);
	// Implemented on the Rust side (src/sim/timing.rs)
	int32_t CrossedIdealFrame(void);
	int32_t IdealFrameTicks(void);
	void StopTimers(void);
	int32_t GetGameClock(void);
	int32_t GetTicksSinceSync(void);
	void ResetSyncTicks(void);
	void ResetClocks(void);
	int32_t GetRealClock(void);

#ifdef __cplusplus
}
#endif

#endif // CLOCK_H
