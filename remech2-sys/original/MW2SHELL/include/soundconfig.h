#ifndef SOUNDCONFIG_H
#define SOUNDCONFIG_H

#include "decomp.h"
#include "types.h"

// SIZE 0x3c
// The sound settings, read from and written to MW2SND.CFG as one block. The options screen's
// sliders and rows point at the members. The names come from the simulator's in-mission menus
// ("SET AUDIO VOLUME", "COMBAT VARIABLES"), which edit the same record.
struct SoundConfig {
	MechS32 m_unk0x00; // 0x00 — the shell never accesses it; the simulator's audio setter handles it with no menu entry
	MechS32 m_effectsVolume;             // 0x04
	MechS32 m_voiceVolume;               // 0x08
	MechS32 m_midiVolume;                // 0x0c
	undefined4 m_unk0x10;                // 0x10 — the simulator's sound flags
	MechS32 m_objectTextmaps;            // 0x14
	MechS32 m_terrainTextmaps;           // 0x18
	MechS32 m_displayDetail;             // 0x1c — high or low
	MechS32 m_objectDensity;             // 0x20 — high or low
	MechS32 m_explosionChunks;           // 0x24
	MechS32 m_displayBrightness;         // 0x28
	MechChar m_videoDriver[0x3c - 0x2c]; // 0x2c — "" or "vesa480.dll"
};

#endif // SOUNDCONFIG_H
