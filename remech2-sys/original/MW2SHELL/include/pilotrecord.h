#ifndef PILOTRECORD_H
#define PILOTRECORD_H

#include "types.h"

class TextGlyph;

// One pilot career record; the roster file MW2REG.CFG holds 20 of them.
struct PilotRecord {
	uint32_t m_inUse;      // 0x00 — 1 for a registered pilot, 0 for an empty slot
	uint32_t m_active;     // 0x04 — the pilot last selected on the roster
	uint32_t m_clan;       // 0x08 — index into g_clanNames: 0 Wolf, 1 Jade Falcon
	int32_t m_mission;     // 0x0c — missions completed, the index of the next
	int32_t m_rank;        // 0x10 — index into g_rankNames
	int32_t m_honor;       // 0x14
	uint32_t m_kills;      // 0x18 — mechs and vehicles
	uint32_t m_hits;       // 0x1c
	uint32_t m_shotsFired; // 0x20
	uint32_t m_unk0x24;    // 0x24 — only cleared, with the statistics before it
	char m_callsign[0x10]; // 0x28
	TextGlyph* m_glyph;    // 0x38 — the callsign on the roster screen, not saved
};

#endif // PILOTRECORD_H
