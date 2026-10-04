#ifndef PILOTRECORD_H
#define PILOTRECORD_H

#include "decomp.h"
#include "types.h"

class TextGlyph;

// SIZE 0x3c
// One pilot career record; the roster file MW2REG.CFG holds 20 of them.
struct PilotRecord {
	undefined4 m_inUse;        // 0x00 — 1 for a registered pilot, 0 for an empty slot
	undefined4 m_active;       // 0x04 — the pilot last selected on the roster
	undefined4 m_clan;         // 0x08 — index into g_clanNames: 0 Wolf, 1 Jade Falcon
	MechS32 m_mission;         // 0x0c — missions completed, the index of the next
	MechS32 m_rank;            // 0x10 — index into g_rankNames
	MechS32 m_honor;           // 0x14
	undefined4 m_kills;        // 0x18 — mechs and vehicles
	undefined4 m_hits;         // 0x1c
	undefined4 m_shotsFired;   // 0x20
	undefined4 m_unk0x24;      // 0x24 — only cleared, with the statistics before it
	MechChar m_callsign[0x10]; // 0x28
	TextGlyph* m_glyph;        // 0x38 — the callsign on the roster screen, not saved
};

#endif // PILOTRECORD_H
