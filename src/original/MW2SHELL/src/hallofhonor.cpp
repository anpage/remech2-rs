#include "hallofhonor.h"

#include "campaignmission.h"
#include "decomp.h"
#include "font.h"
#include "keyboardinput.h"
#include "loopingmovie.h"
#include "menudata.h"
#include "mousestate.h"
#include "pilotrecord.h"
#include "refreshmode.h"
#include "shellglobals.h"
#include "shellmain.h"
#include "types.h"
#include "video.h"
#include "videodriver.h"
#include "windowstate.h"

void operator delete(void*);

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// FUNCTION: MW2SHELL 0x1003e2f0
MechS32 ComparePilotRecords(const PilotRecord** p_first, const PilotRecord** p_second)
{
	const PilotRecord** firstParam = p_first;
	const PilotRecord** secondParam = p_second;
	const PilotRecord* first = *firstParam;
	const PilotRecord* second = *secondParam;

	if (second->m_rank < first->m_rank) {
		return -1;
	}
	if (second->m_rank > first->m_rank) {
		return 1;
	}
	if (second->m_honor < first->m_honor) {
		return -1;
	}
	if (second->m_honor > first->m_honor) {
		return 1;
	}
	if (second->m_mission < first->m_mission) {
		return -1;
	}
	if (second->m_mission > first->m_mission) {
		return 1;
	}

	return 0;
}

void HallOfHonorCallback(MechS32 p_active);

// GLOBAL: MW2SHELL 0x1006aeac
LoopingMovie* g_hallOfHonorMovie = NULL;

// GLOBAL: MW2SHELL 0x10090670
MechChar g_hallOfHonorText[0x20];

// Draw the eight highest-ranking active pilots from both clan rosters.
// FUNCTION: MW2SHELL 0x1003e3c9
void DrawHallOfHonor()
{
	PilotRecord* pilots[20];
	MechS32 i;
	MechS32 top;

	g_videoDriver->GetPalette(g_savedScreenPalette);
	g_keyboardInput->FlushKeys();
	g_videoDriver->LoadPalette(2);
	g_videoDriver->m_restoreColor = 0;
	g_hallOfHonorMovie = new LoopingMovie("amwlogo1", 0x78, 4);

	for (i = 0; i < 20; i++) {
		pilots[i] = &g_pilotRoster[i];
	}
	qsort(pilots, 20, sizeof(pilots[0]), (int (*)(const void*, const void*)) ComparePilotRecords);

	top = 0x96;
	g_videoDriver->DrawString(0, top, g_buttonFont->m_dataCopy, "Pilot", NULL);
	g_videoDriver->DrawString(0x7d, top, g_buttonFont->m_dataCopy, "Clan", NULL);
	g_videoDriver->DrawString(200, top, g_buttonFont->m_dataCopy, "Rank", NULL);
	g_videoDriver->DrawString(0x145, top, g_buttonFont->m_dataCopy, "Honor", NULL);
	g_videoDriver->DrawString(400, top, g_buttonFont->m_dataCopy, "Kills", NULL);
	g_videoDriver->DrawString(0x1cc, top, g_buttonFont->m_dataCopy, "Hit %", NULL);
	g_videoDriver->DrawString(0x208, top, g_buttonFont->m_dataCopy, "Last Mission", NULL);
	top += 0x20;

	for (i = 0; i < 8; i++) {
		if (!pilots[i]->m_inUse) {
			continue;
		}

		g_videoDriver->DrawString(0, top, g_defaultFont->m_dataCopy, pilots[i]->m_callsign, NULL);
		g_videoDriver->DrawString(0x7d, top, g_defaultFont->m_dataCopy, g_clanNames[pilots[i]->m_clan], NULL);
		g_videoDriver->DrawString(200, top, g_defaultFont->m_dataCopy, g_rankNames[pilots[i]->m_rank], NULL);
		sprintf(g_hallOfHonorText, "%d", pilots[i]->m_honor);
		g_videoDriver->DrawString(0x145, top, g_defaultFont->m_dataCopy, g_hallOfHonorText, NULL);
		sprintf(g_hallOfHonorText, "%d", pilots[i]->m_kills);
		g_videoDriver->DrawString(400, top, g_defaultFont->m_dataCopy, g_hallOfHonorText, NULL);
		if (pilots[i]->m_shotsFired != 0) {
			sprintf(g_hallOfHonorText, "%d%%", (MechS32) pilots[i]->m_hits * 100 / (MechS32) pilots[i]->m_shotsFired);
		}
		else {
			strcpy(g_hallOfHonorText, "-");
		}
		g_videoDriver->DrawString(0x1cc, top, g_defaultFont->m_dataCopy, g_hallOfHonorText, NULL);
		if (pilots[i]->m_mission == 0) {
			g_videoDriver->DrawString(0x208, top, g_defaultFont->m_dataCopy, "----", NULL);
		}
		else {
			g_videoDriver->DrawString(
				0x208,
				top,
				g_defaultFont->m_dataCopy,
				g_campaignMissions[pilots[i]->m_clan][pilots[i]->m_mission - 1].m_title,
				NULL
			);
		}
		top += 0x10;
	}

	RegisterMenuFunction(HallOfHonorCallback);
}

// FUNCTION: MW2SHELL 0x1003e86b
void HallOfHonorCallback(MechS32 p_active)
{
	if (p_active && g_hallOfHonorMovie != NULL) {
		g_hallOfHonorMovie->Update();
	}
	if (!p_active || g_mouseState->GetRightPressed() == 1 || g_mouseState->GetLeftPressed() == 1 ||
		g_keyboardInput->PollKey() != 0) {
		UnregisterMenuFunction(HallOfHonorCallback);
		EnableMenuItem(g_windowMenu, c_menuHallOfHonor, MF_ENABLED);
		g_menuDialogOpen = FALSE;
		if (g_hallOfHonorMovie != NULL) {
			delete g_hallOfHonorMovie;
		}
		g_hallOfHonorMovie = NULL;
		g_videoDriver->m_restoreColor = -1;
		g_videoDriver->RestoreBackground(0, 0, 0x280, 0x1e0);
		UpdateVideos();
		g_videoDriver->SetPalette(g_savedScreenPalette, TRUE);
		if (p_active) {
			g_videoDriver->DrawShell();
			g_videoDriver->RestoreBackground(0, 0, 0x280, 0x1e0);
			UpdateVideos();
		}
	}
}
