#ifndef PILOTRECORD_H
#define PILOTRECORD_H

#include "types.h"

class TextGlyph;

// One pilot career record; the roster file MW2REG.CFG holds 20 of them.
struct PilotRecord {
	uint32_t m_inUse;  // 1 for a registered pilot, 0 for an empty slot
	uint32_t m_active; // the pilot last selected on the roster
	uint32_t m_clan;   // index into g_clanNames: 0 Wolf, 1 Jade Falcon
	int32_t m_mission; // missions completed, the index of the next
	int32_t m_rank;    // index into g_rankNames
	int32_t m_honor;
	uint32_t m_kills; // mechs and vehicles
	uint32_t m_hits;
	uint32_t m_shotsFired;
	uint32_t m_reserved0x24; // cleared with the statistics before it
	char m_callsign[0x10];
	TextGlyph* m_glyph; // the callsign on the roster screen, not saved
};

#endif // PILOTRECORD_H
