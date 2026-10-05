#ifndef EFFECTINFO_H
#define EFFECTINFO_H

#include "decomp.h"
#include "types.h"

// The properties of an effect type (g_effectInfo).
// SIZE 0x1c
typedef struct EffectInfo {
	MechS32 m_duration;    // 0x00 — ticks
	MechS32 m_flash;       // 0x04 — a palette flash (StartPaletteFlash) when the camera follows, or -1
	MechS32 m_sound;       // 0x08 — a sound effect, or <= 0
	MechS32 m_soundChance; // 0x0c — the percent chance of the sound, or -1 for always
	MechS32 m_camera;      // 0x10 — the effect camera may follow it
	MechS32 m_needsObject; // 0x14 — only a slot with a scene object can play it
	MechS32 m_damages;     // 0x18 — it deals splash damage while it lasts
} EffectInfo;

#endif // EFFECTINFO_H
