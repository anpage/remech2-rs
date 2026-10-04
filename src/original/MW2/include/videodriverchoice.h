#ifndef VIDEODRIVERCHOICE_H
#define VIDEODRIVERCHOICE_H

#include "types.h"

// The video driver the game uses: MW2SND.CFG's, or the command line's /VG=name.
typedef struct VideoDriverChoice {
	MechU32 m_flags; // 0x00 — 1: m_name is set
	char m_name[13]; // 0x04 — empty to scan for one
} VideoDriverChoice;

#endif // VIDEODRIVERCHOICE_H
