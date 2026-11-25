#ifndef EFFECT_H
#define EFFECT_H

#include "decomp.h"
#include "object.h"
#include "types.h"

// An explosion, fire or other visual effect in the world (g_effects). Each slot keeps the scene
// object for its type once one is created.
typedef struct Effect {
	SceneObject* m_object; // 0x00
	int32_t m_position[3]; // 0x04
	int32_t m_timeLeft;    // 0x10 — ticks
	int32_t m_active;      // 0x14
	int32_t m_animation;   // 0x18 — the animation it plays, or -1
	int32_t m_hasCamera;   // 0x1c — the effect camera follows it
	int32_t m_type;        // 0x20 — an index into g_effectInfo
	int32_t m_owner;       // 0x24 — the player it damages on behalf of, or -2
} Effect;

#endif // EFFECT_H
