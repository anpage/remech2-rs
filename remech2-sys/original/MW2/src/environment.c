/* The mission's time of day: g_timeOfDay counts seconds of a g_secondsPerDay-second day, and
   the four phases that start at g_timeOfDayStarts each fade to their own palette. */
#include "environment.h"

#include "clock.h"
#include "cockpitpanel.h"
#include "config.h"
#include "fixeddiv.h"
#include "mech.h"
#include "palette.h"
#include "players.h"
#include "soundfx.h"
#include "speech.h"
#include "types.h"

TimeOfDayPhase g_timeOfDayPhases[4] = {{4, 2715}, {0, 2715}, {4, 3620}, {8, 2715}};

int32_t g_timeOfDayPhase = -1;

int32_t g_soundDelayPerUnit = 0x168;

int32_t g_gravity = 0x794;

int32_t g_gravityScale = 0x10000;

int32_t g_secondsPerDay = 86400;

int32_t g_daysPerYear = 365;

int32_t g_dayOfYear = 0;

int32_t g_timeOfDay = 43200;

int32_t g_startTimeOfDay = 43200;

int32_t g_timeOfDayFrames;

int32_t g_timeOfDayStarts[4];

int32_t g_timeOfDayEnabled;

int32_t g_nextTimeOfDayUpdate;

int32_t g_infraredOn;

void FirstEnvironment(void)
{
	int32_t hour;

	g_timeOfDayEnabled = 1;
	hour = g_secondsPerDay / 24;
	g_timeOfDayStarts[0] = hour * 5;
	g_timeOfDayStarts[1] = hour * 7;
	g_timeOfDayStarts[2] = hour * 17;
	g_timeOfDayStarts[3] = hour * 19;
	if (g_timeOfDayPhase != -1) {
		g_timeOfDay = g_timeOfDayStarts[g_timeOfDayPhase];
	}

	g_startTimeOfDay = g_timeOfDay;
	g_timeOfDayPhase = -1;
	g_gravityScale = FixedDiv16(g_gravity, 0x794);
}

void UpdateTimeOfDay(void)
{
	int32_t phase;
	int32_t seconds;
	int32_t time;
	int32_t i;

	phase = 3;
	if (g_cockpitPanels[c_panelRadar]->m_damage >= 1 && g_infraredOn == 1) {
		SetInfrared(0);
		g_infraredOn = 0;
	}

	if (g_timeOfDayFrames < 2) {
		g_timeOfDayFrames++;
		g_nextTimeOfDayUpdate = 0;
	}
	else {
		if (g_currentClock < g_nextTimeOfDayUpdate) {
			return;
		}

		g_nextTimeOfDayUpdate = g_currentClock + 0x712;
		seconds = g_currentClock / 181;
		time = (g_startTimeOfDay + seconds) % g_secondsPerDay;
		if (time < g_timeOfDay) {
			g_dayOfYear++;
			g_dayOfYear %= g_daysPerYear;
		}

		g_timeOfDay = time;
		for (i = 0; i <= 3; i++) {
			if (g_timeOfDayStarts[i] <= g_timeOfDay) {
				phase = i;
			}
		}

		FadeToTimeOfDayPhase(phase);
	}
}

void FadeToTimeOfDayPhase(int32_t p_phase)
{
	int32_t duration;

	if (g_timeOfDayEnabled == 1 && p_phase != g_timeOfDayPhase) {
		if (g_infraredOn == 1) {
			duration = 181;
			g_infraredOn = 0;
		}
		else {
			duration = g_timeOfDayPhases[p_phase].m_duration;
		}

		if (g_timeOfDayFrames == 2) {
			g_timeOfDayFrames++;
			duration = 362;
		}

		FadeToBasePalette(g_timeOfDayPhases[p_phase].m_palette, duration);
		g_timeOfDayPhase = p_phase;
	}
}

int32_t IsInfraredOn(void)
{
	return g_infraredOn;
}

void SetInfrared(int32_t p_state)
{
	if (g_infraredOn != p_state) {
		if (p_state == 1) {
			if (g_cockpitPanels[c_panelRadar]->m_damage < 1) {
				g_infraredOn = 1;
				g_timeOfDayEnabled = 0;
				FadeToBasePalette(12, 181);
				PlaySoundEffect(0xb2, 100, 0x40, 5, 0x50);
				PlayCockpitSound(0x1c, 1);
			}
		}
		else if (p_state == 0) {
			g_timeOfDayEnabled = 1;
			g_timeOfDayPhase = -1;
			g_nextTimeOfDayUpdate = 0;
		}
	}
}

void QuadrupleMechCooling(void)
{
	int32_t i;
	Mech* mech;

	for (i = 0; i < g_playerCount; i++) {
		mech = g_players[i]->m_mech;
		mech->m_cooling <<= 2;
	}
}

void QuarterMechCooling(void)
{
	int32_t i;
	Mech* mech;

	for (i = 0; i < g_playerCount; i++) {
		mech = g_players[i]->m_mech;
		mech->m_cooling >>= 2;
	}
}
