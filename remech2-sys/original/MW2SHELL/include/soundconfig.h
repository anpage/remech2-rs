#ifndef SOUNDCONFIG_H
#define SOUNDCONFIG_H

#include "types.h"

// The sound and display settings, copied field by field to and from the game settings
// (LoadSoundConfig, SaveSoundConfig). The options screen's sliders and rows point at the members.
// The names come from the simulator's in-mission menus ("SET AUDIO VOLUME", "COMBAT VARIABLES").
struct SoundConfig {
	int32_t m_effectsVolume;
	int32_t m_voiceVolume;
	int32_t m_midiVolume;
	int32_t m_objectTextmaps;
	int32_t m_terrainTextmaps;
	// high or low
	int32_t m_displayDetail;
	// high or low
	int32_t m_objectDensity;
	int32_t m_explosionChunks;
	int32_t m_displayBrightness;
};

#endif // SOUNDCONFIG_H
