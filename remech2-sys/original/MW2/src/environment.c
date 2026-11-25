/* The mission's time of day: g_timeOfDay counts seconds of a g_secondsPerDay-second day, and
   the four phases that start at g_timeOfDayStarts each fade to their own palette. */
#include "environment.h"

#include "clock.h"
#include "cockpitpanel.h"
#include "config.h"
#include "decomp.h"
#include "fixeddiv.h"
#include "mech.h"
#include "palette.h"
#include "players.h"
#include "soundfx.h"
#include "speech.h"
#include "types.h"

TimeOfDayPhase g_timeOfDayPhases[4] = {{4, 2715}, {0, 2715}, {4, 3620}, {8, 2715}};

MechS32 g_timeOfDayPhase = -1;

MechS32 g_soundDelayPerUnit = 0x168;

MechS32 g_gravity = 0x794;

MechS32 g_gravityScale = 0x10000;

MechS32 g_unk0x100ba608 = 0;

MechS32 g_secondsPerDay = 86400;

MechS32 g_daysPerYear = 365;

MechS32 g_dayOfYear = 0;

MechS32 g_timeOfDay = 43200;

MechS32 g_startTimeOfDay = 43200;

MechS32 g_timeOfDayFrames;

MechS32 g_timeOfDayStarts[4];

MechS32 g_timeOfDayEnabled;

MechS32 g_nextTimeOfDayUpdate;

MechS32 g_infraredOn;

void FirstEnvironment(void)
{
	MechS32 hour;

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
	MechS32 phase;
	MechS32 seconds;
	MechS32 time;
	MechS32 i;

	phase = 3;
	if (g_cockpitPanels[c_panelRadar]->m_damage >= 1 && g_infraredOn == 1) {
		SetInfrared(0, 0);
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

void FadeToTimeOfDayPhase(MechS32 p_phase)
{
	MechS32 duration;

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

MechS32 IsInfraredOn(undefined4 p_unk0x00)
{
	return g_infraredOn;
}

void SetInfrared(undefined4 p_unk0x00, MechS32 p_state)
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
	MechS32 i;
	Mech* mech;

	for (i = 0; i < g_playerCount; i++) {
		mech = g_players[i]->m_mech;
		mech->m_cooling <<= 2;
	}
}

void QuarterMechCooling(void)
{
	MechS32 i;
	Mech* mech;

	for (i = 0; i < g_playerCount; i++) {
		mech = g_players[i]->m_mech;
		mech->m_cooling >>= 2;
	}
}
