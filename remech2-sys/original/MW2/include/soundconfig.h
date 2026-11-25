#ifndef SOUNDCONFIG_H
#define SOUNDCONFIG_H

#include "types.h"

// The sound and display settings, copied field by field to and from the player's settings.
struct SoundConfig {
	int32_t m_effectsVolume;
	int32_t m_voiceVolume;
	// the simulator plays its music from CD
	int32_t m_midiVolume;
	// the simulator's sound flags
	uint32_t m_simFlags;
	int32_t m_objectTextmaps;
	int32_t m_terrainTextmaps;
	// high or low
	int32_t m_displayDetail;
	// high or low
	int32_t m_objectDensity;
	int32_t m_explosionChunks;
	int32_t m_displayBrightness;
};
typedef struct SoundConfig SoundConfig;

#endif // SOUNDCONFIG_H
