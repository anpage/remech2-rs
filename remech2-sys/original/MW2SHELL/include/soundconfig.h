#ifndef SOUNDCONFIG_H
#define SOUNDCONFIG_H

#include "decomp.h"
#include "types.h"

// The sound and display settings, copied field by field to and from the game settings
// (LoadSoundConfig, SaveSoundConfig). The options screen's sliders and rows point at the members.
// The names come from the simulator's in-mission menus ("SET AUDIO VOLUME", "COMBAT VARIABLES").
struct SoundConfig {
	MechS32 m_effectsVolume;
	MechS32 m_voiceVolume;
	MechS32 m_midiVolume;
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
