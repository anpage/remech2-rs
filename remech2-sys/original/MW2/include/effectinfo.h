#ifndef EFFECTINFO_H
#define EFFECTINFO_H

#include "types.h"

// The properties of an effect type (g_effectInfo).
typedef struct EffectInfo {
	int32_t m_duration;    // ticks
	int32_t m_flash;       // a palette flash (StartPaletteFlash) when the camera follows, or -1
	int32_t m_sound;       // a sound effect, or <= 0
	int32_t m_soundChance; // the percent chance of the sound, or -1 for always
	int32_t m_camera;      // the effect camera may follow it
	int32_t m_needsObject; // only a slot with a scene object can play it
	int32_t m_damages;     // it deals splash damage while it lasts
} EffectInfo;

#endif // EFFECTINFO_H
