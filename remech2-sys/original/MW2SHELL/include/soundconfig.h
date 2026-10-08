#ifndef SOUNDCONFIG_H
#define SOUNDCONFIG_H

#include "decomp.h"
#include "types.h"

// The sound settings, read from and written to MW2SND.CFG as one block. The options screen's
// sliders and rows point at the members. The names come from the simulator's in-mission menus
// ("SET AUDIO VOLUME", "COMBAT VARIABLES"), which edit the same record.
struct SoundConfig {
	// the shell never accesses it; the simulator's audio setter handles it with no menu entry
	MechS32 m_unk0x00;
	MechS32 m_effectsVolume;
	MechS32 m_voiceVolume;
	MechS32 m_midiVolume;
	// the simulator's sound flags
	undefined4 m_unk0x10;
	MechS32 m_objectTextmaps;
	MechS32 m_terrainTextmaps;
	// high or low
	MechS32 m_displayDetail;
	// high or low
	MechS32 m_objectDensity;
	MechS32 m_explosionChunks;
	MechS32 m_displayBrightness;
};

#endif // SOUNDCONFIG_H
