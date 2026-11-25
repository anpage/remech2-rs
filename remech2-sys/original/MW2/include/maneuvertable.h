#ifndef MANEUVERTABLE_H
#define MANEUVERTABLE_H

#include "types.h"

// An entry of an AI maneuver table: a maneuver ([0]), how many may follow it ([1]) and those
// maneuvers (from [2]).
typedef struct ManeuverEntry {
	int16_t m_list[9];
} ManeuverEntry;

// An AI maneuver table (ChooseManeuver): the maneuvers a player type chooses among.
typedef struct ManeuverTable {
	int16_t m_count;
	int16_t m_followOnly; // the last entries, which never start a run of maneuvers
	ManeuverEntry* m_entries;
} ManeuverTable;

#endif // MANEUVERTABLE_H
