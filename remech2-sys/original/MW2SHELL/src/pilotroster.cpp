#include "pilotroster.h"

#include "debugprint.h"
#include "decomp.h"
#include "files.h"
#include "pilotrecord.h"
#include "shellglobals.h"
#include "types.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

// MW2REG.CFG's records are 0x3c bytes: a PilotRecord up to m_glyph, then the 32-bit pointer the
// original saved with it, which nothing reads. The original read and wrote the array whole.
static const size_t c_savedPilotSize = offsetof(PilotRecord, m_glyph);
static const long c_savedGlyphSize = 4;

// Stack-slot permutation: file and i swap [ebp-N] slots with the original.
// FUNCTION: MW2SHELL 0x1002da80
void LoadPilotRoster()
{
	PilotRecord* pilot;
	FILE* file;
	MechS32 i;

	file = MechFopen("MW2REG.CFG", "rb");
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
		for (i = 0; i < 20; i++) {
			fread(&g_pilotRoster[i], c_savedPilotSize, 1, file);
			fseek(file, c_savedGlyphSize, SEEK_CUR);
		}

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
	static const MechU8 glyph[4] = {0};
	FILE* file;
	MechS32 i;

	file = MechFopen("MW2REG.CFG", "wb");
	if (file == NULL) {
		ShowMessage("Error Writing Career File\n");
		return;
	}

	for (i = 0; i < 20; i++) {
		fwrite(&g_pilotRoster[i], c_savedPilotSize, 1, file);
		fwrite(glyph, c_savedGlyphSize, 1, file);
	}

	fclose(file);
}
