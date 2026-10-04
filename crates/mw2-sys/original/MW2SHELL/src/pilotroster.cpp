#include "pilotroster.h"

#include "debugprint.h"
#include "decomp.h"
#include "pilotrecord.h"
#include "shellglobals.h"
#include "types.h"

#include <stdio.h>
#include <string.h>

// Stack-slot permutation: file and i swap [ebp-N] slots with the original.
// FUNCTION: MW2SHELL 0x1002da80
void LoadPilotRoster()
{
	PilotRecord* pilot;
	FILE* file;
	MechS32 i;

	file = fopen("MW2REG.CFG", "rb");
	if (file == NULL) {
		for (i = 0; i < 20; i++) {
			pilot = &g_pilotRoster[i];
			pilot->m_inUse = 0;
			pilot->m_active = 0;
			if (i >= 10) {
				pilot->m_clan = 1;
			}
			else {
				pilot->m_clan = 0;
			}
			pilot->m_mission = 0;
			pilot->m_rank = 0;
			pilot->m_honor = 0;
			pilot->m_kills = 0;
			pilot->m_hits = 0;
			pilot->m_shotsFired = 0;
			pilot->m_unk0x24 = 0;
			strcpy(pilot->m_callsign, "");
		}
	}
	else {
		fread(g_pilotRoster, 0x3c, 20, file);
		fclose(file);
	}

	for (i = 0; i < 20; i++) {
		pilot = &g_pilotRoster[i];
		pilot->m_glyph = NULL;
	}
}

// FUNCTION: MW2SHELL 0x1002dbec
void SavePilotRoster()
{
	FILE* file;

	file = fopen("MW2REG.CFG", "wb");
	if (file == NULL) {
		ShowMessage("Error Writing Career File\n");
		return;
	}

	fwrite(g_pilotRoster, 0x3c, 20, file);
	fclose(file);
}
