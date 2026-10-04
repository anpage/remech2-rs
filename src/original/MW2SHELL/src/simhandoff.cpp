#include "simhandoff.h"

#include "decomp.h"
#include "mechvariant.h"
#include "pilotrecord.h"
#include "refreshmode.h"
#include "shellglobals.h"
#include "shellmain.h"
#include "simhandoffstate.h"
#include "types.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>

DECOMP_SIZE_ASSERT(SimHandoffState, 0x218)

// The mission's name, from its BWD file (ShellApplyMissionUiInfo), passed to the simulator as
// "-b=".
// GLOBAL: MW2SHELL 0x1006a550
MechChar g_missionName[0x10] = "xxxxxxxx.xxx";

// GLOBAL: MW2SHELL 0x10090288
SimHandoffState g_simHandoff;

// Reads the shell's state back from mw2prm.cfg after a mission. With p_fromSim, posts the saved
// message to the shell window; otherwise it returns to the campaign's start (2, no pilot, no
// scenario).
// FUNCTION: MW2SHELL 0x10039b50
void ReadSimHandoff(BOOL p_fromSim, MechS32* p_campaign, MechU8* p_pilotChosen, char** p_scenario)
{
	MechS32 i;
	FILE* file;

	file = fopen("mw2prm.cfg", "rb");
	if (!file) {
		return;
	}

	if (fread(&g_simHandoff, sizeof(g_simHandoff), 1, file) != 1) {
		fclose(file);
		return;
	}
	fclose(file);

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
		PostMessage(g_gameWindow, g_simHandoff.m_msg, c_msgLaunchSim, 0);
	}
	else {
		*p_campaign = 2;
		*p_pilotChosen = 0;
		*p_scenario = NULL;
	}
}

// Saves the shell's state to mw2prm.cfg before a mission: the message to post on return, the
// campaign, the pilot, and the simulator's command line (the scenario and "-b=" the mission's
// name).
// FUNCTION: MW2SHELL 0x10039c92
void WriteSimHandoff(UINT p_msg, MechS32 p_campaign, MechU8 p_pilotChosen, const char* p_scenario)
{
	FILE* file;

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

	file = fopen("mw2prm.cfg", "wb");
	if (!file) {
		return;
	}

	fwrite(&g_simHandoff, sizeof(g_simHandoff), 1, file);
	fclose(file);
}
