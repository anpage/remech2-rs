#ifndef SOUNDCONFIG_H
#define SOUNDCONFIG_H

#include "decomp.h"
#include "types.h"

// The sound and display settings, copied field by field to and from the player's settings.
struct SoundConfig {
	MechS32 m_effectsVolume;
	MechS32 m_voiceVolume;
	// the simulator plays its music from CD
	MechS32 m_midiVolume;
	// the simulator's sound flags
	undefined4 m_simFlags;
	MechS32 m_objectTextmaps;
	MechS32 m_terrainTextmaps;
	// high or low
	MechS32 m_displayDetail;
	// high or low
	MechS32 m_objectDensity;
	MechS32 m_explosionChunks;
	MechS32 m_displayBrightness;
};
typedef struct SoundConfig SoundConfig;

#endif // SOUNDCONFIG_H
