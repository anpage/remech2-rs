#include "simhandoff.h"

#include "mechvariant.h"
#include "messages.h"
#include "shellglobals.h"
#include "shellmain.h"
#include "simhandoffstate.h"
#include "types.h"

#include <stdio.h>
#include <string.h>

// The mission's name, from its BWD file (ShellApplyMissionUiInfo), passed to the simulator as
// "-b=".
char g_missionName[0x10] = "xxxxxxxx.xxx";

SimHandoffState g_simHandoff;

// Reads the shell's state back from Rust after a mission. With p_fromSim, posts the saved
// message to the shell window; otherwise it returns to the campaign's start (2, no pilot, no
// scenario).
void ReadSimHandoff(int32_t p_fromSim, int32_t* p_campaign, uint8_t* p_pilotChosen, char** p_scenario)
{
	int32_t i;

	if (!MechLoadSimHandoff(&g_simHandoff)) {
		return;
	}

	*p_campaign = g_simHandoff.m_campaign;
	*p_pilotChosen = g_simHandoff.m_pilotChosen;
	*p_scenario = g_simHandoff.m_cmdLine;
	for (i = 0; g_simHandoff.m_cmdLine[i] > ' '; i++) {
	}
	g_simHandoff.m_cmdLine[i] = '\0';

	if (g_simHandoff.m_pilot >= 0) {
		g_currentPilot = &g_pilotRoster[g_simHandoff.m_pilot];
	}
	else {
		g_currentPilot = NULL;
	}
	RestoreStars();

	if (p_fromSim) {
		MechPostMessage(g_simHandoff.m_msg, c_msgLaunchSim, 0);
	}
	else {
		*p_campaign = 2;
		*p_pilotChosen = 0;
		*p_scenario = NULL;
	}
}

// Saves the shell's state to Rust before a mission: the message to post on return, the
// campaign, the pilot, and the simulator's command line (the scenario and "-b=" the mission's
// name).
void WriteSimHandoff(uint32_t p_msg, int32_t p_campaign, uint8_t p_pilotChosen, const char* p_scenario)
{
	g_simHandoff.m_msg = p_msg;
	g_simHandoff.m_campaign = p_campaign;
	g_simHandoff.m_pilotChosen = p_pilotChosen;
	strcpy(g_simHandoff.m_cmdLine, p_scenario);
	strcat(g_simHandoff.m_cmdLine, " -b=");
	strcat(g_simHandoff.m_cmdLine, g_missionName);

	if (g_currentPilot) {
		g_simHandoff.m_pilot = g_currentPilot - g_pilotRoster;
	}
	else {
		g_simHandoff.m_pilot = -1;
	}
	SaveStars();

	if (p_msg != c_msgCadetTraining && p_msg != c_msgQuit) {
		WriteStarFiles();
	}

	MechSaveSimHandoff(&g_simHandoff);
}
