#include "clock.h"

#include "debugprint.h"
#include "mss.h"
#include "network.h"
#include "ticks.h"
#include "transform.h"
#include "types.h"

#include <math.h>

// The game clock, in the 181 Hz ticks of ticks.h, which FirstClock starts.

int32_t g_currentClock = 0;

// Ticks that run on while the game clock is paused; the network times its messages by them.
int32_t g_realClock = 0;

int32_t g_deltaTime = 0;

// 0 the game clock runs on its own ticks, 2 a slave's follows the master's, 3 paused.
int32_t g_clockMode = 0;

int32_t g_clockPaused = 0;

int32_t g_timeCompressionEnabled = 0;

int32_t g_timeExpansionEnabled = 0;

int32_t g_framerateLimit = 0;

int32_t g_clockHandle = -1;

// The ticks since a slave's last UpdateNetwork: they advance its game clock when the master's
// clock didn't arrive.
int32_t g_syncTicksHandle = -1;

int32_t g_realClockHandle = -1;

int32_t g_previousClock = 0;

int32_t g_clockModeBeforePause = 0;

int32_t g_ticksTimerInitialized = false;

int16_t* g_sqrtTable;

int32_t g_slopeSines[800];

int32_t g_slopeCosines[800];

int32_t g_sinTable[0x102];

int16_t g_sqrtTableData[0x400];

int32_t g_atanTable[0x102];

// A quarter wave of sines (2.29 fixed point, 1024 steps to the circle) and the arctangents of
// 0 to 1 in 256 steps (16.16 degrees), each padded with two copies of its last value.
int32_t InitSinAtanTables(void)
{
	int32_t i;

	for (i = 0; i < 0x100; i++) {
		g_sinTable[i] = (int32_t) (sin(i * (3.14159265 / 512)) * 536870912.0);
		g_atanTable[i] = (int32_t) (atan(i / 256.0) * 3754936.210460003);
	}

	g_sinTable[0x100] = g_sinTable[0x101] = 0x20000000;
	g_atanTable[0x100] = g_atanTable[0x101] = 0x2d0000;
	return true;
}

// The cosines and sines of the angles whose tangents are 0 to 50 in steps of 1/16 (2.29).
int32_t InitSlopeTables(void)
{
	int32_t i;
	double cosine;

	for (i = 0; i < 800; i++) {
		g_slopeCosines[i] = (int32_t) ((cosine = 1.0 / sqrt(i / 16.0 * (i / 16.0) + 1.0)) * 536870912.0);
		g_slopeSines[i] = (int32_t) (i / 16.0 * cosine * 536870912.0);
	}

	return true;
}

// The square roots of 0 to 1023 (6.10 fixed point).
int32_t InitSqrtTable(void)
{
	int32_t i;

	g_sqrtTable = g_sqrtTableData;
	for (i = 0; i < 0x400; i++) {
		g_sqrtTable[i] = (int16_t) (sqrt(i) * 1024.0);
	}

	return true;
}

int32_t Hypot2D(int32_t p_x, int32_t p_y)
{
	int32_t length;
	double x;
	double y;

	x = p_x;
	// The original assigned y inside the product, which C leaves unsequenced
	y = p_y;
	length = (int32_t) sqrt(y * y + x * x);
	return length;
}

// Sets p_matrix to the rotation that points along (p_x, p_y, p_z).
void BuildMatrixFromDirection(Matrix* p_matrix, int32_t p_x, int32_t p_y, int32_t p_z)
{
	double pitch;
	double yaw;

	yaw = atan2(p_x, p_z);
	pitch = -asin(p_y / sqrt((double) p_x * p_x + (double) p_y * p_y + (double) p_z * p_z));
	BuildMatrix(p_matrix, (int32_t) (pitch * 3754939.378), (int32_t) (yaw * 3754939.378), 0, 0, 0, 0);
}

// Normalizes the rows and columns of the rotation (2.29 fixed point).
void NormalizeRotation(Matrix* p_matrix)
{
	ScaleVectorToLength(0x20000000, &p_matrix->m_rows[0][0], &p_matrix->m_rows[0][1], &p_matrix->m_rows[0][2]);
	ScaleVectorToLength(0x20000000, &p_matrix->m_rows[1][0], &p_matrix->m_rows[1][1], &p_matrix->m_rows[1][2]);
	ScaleVectorToLength(0x20000000, &p_matrix->m_rows[2][0], &p_matrix->m_rows[2][1], &p_matrix->m_rows[2][2]);
	ScaleVectorToLength(0x20000000, &p_matrix->m_rows[0][0], &p_matrix->m_rows[1][0], &p_matrix->m_rows[2][0]);
	ScaleVectorToLength(0x20000000, &p_matrix->m_rows[0][1], &p_matrix->m_rows[1][1], &p_matrix->m_rows[2][1]);
	ScaleVectorToLength(0x20000000, &p_matrix->m_rows[0][2], &p_matrix->m_rows[1][2], &p_matrix->m_rows[2][2]);
}

// Scales (*p_x, *p_y, *p_z) to length p_length.
void ScaleVectorToLength(int32_t p_length, int32_t* p_x, int32_t* p_y, int32_t* p_z)
{
	double scale;
	double x;
	double y;
	double z;

	x = *p_x;
	y = *p_y;
	z = *p_z;
	*p_x = (int32_t) ((scale = p_length / sqrt(x * x + y * y + z * z)) * x);
	*p_y = (int32_t) (y * scale);
	*p_z = (int32_t) (z * scale);
}

void FirstClock(void)
{
	if (!g_ticksTimerInitialized) {
		StartTicks();
		g_realClockHandle = AllocTicks(0x100);
		g_clockHandle = AllocTicks(0x80);
		g_syncTicksHandle = AllocTicks(0x80);
		ResetTicks(g_realClockHandle);
		ResetTicks(g_syncTicksHandle);
		ResetTicks(g_clockHandle);
		g_ticksTimerInitialized = true;
	}

	g_currentClock = g_previousClock = 0;
	g_realClock = 0;
}

// NextClock on the Rust side (src/sim/timing.rs) wraps it, limiting the framerate.
void NextClockC(void)
{
	if (!g_ticksTimerInitialized) {
		return;
	}

	if (g_clockMode == 0) {
		g_currentClock = GetTicks(g_clockHandle);
		if (g_framerateLimit > 0) {
			while (g_currentClock - g_previousClock < g_framerateLimit) {
				g_currentClock = GetTicks(g_clockHandle);
			}
		}

		if (g_clockPaused) {
			g_clockModeBeforePause = g_clockMode;
			g_clockMode = 3;
			DebugPrint("NextClock(1): pause_timer(TRUE)");
			PauseTimer(0x80, true);
		}
	}
	else if (g_clockMode == 3) {
		g_currentClock += 12;
		if (!g_clockPaused) {
			g_clockMode = g_clockModeBeforePause;
			DebugPrint("NextClock(2): pause_timer(FALSE)");
			PauseTimer(0x80, false);
			SetTicks(g_clockHandle, g_currentClock);
		}
	}

	g_deltaTime = g_currentClock - g_previousClock;
	if (!g_isNetworkGame) {
		if (g_timeCompressionEnabled) {
			g_deltaTime <<= 3;
			g_currentClock += g_deltaTime;
			SetTicks(g_clockHandle, g_currentClock);
		}
		else if (g_timeExpansionEnabled) {
			g_deltaTime >>= 2;
			g_currentClock += g_deltaTime;
			SetTicks(g_clockHandle, g_currentClock);
		}
	}

	if (g_deltaTime <= 0) {
		g_deltaTime = 0;
		g_currentClock = g_previousClock;
	}

	g_previousClock = g_currentClock;
}

void StopTimers(void)
{
	if (g_ticksTimerInitialized) {
		FreeTicks(g_syncTicksHandle);
		FreeTicks(g_clockHandle);
		FreeTicks(g_realClockHandle);
		StopTicks();
		AIL_shutdown();
		g_ticksTimerInitialized = false;
	}
}

int32_t GetGameClock(void)
{
	return GetTicks(g_clockHandle);
}

int32_t GetTicksSinceSync(void)
{
	return GetTicks(g_syncTicksHandle);
}

void ResetSyncTicks(void)
{
	ResetTicks(g_syncTicksHandle);
}

void ResetClocks(void)
{
	ResetTicks(g_realClockHandle);
	ResetTicks(g_syncTicksHandle);
	ResetTicks(g_clockHandle);
	g_currentClock = g_previousClock = 0;
}

int32_t GetRealClock(void)
{
	return GetTicks(g_realClockHandle);
}
