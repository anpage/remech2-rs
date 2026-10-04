#ifndef EFFECT_H
#define EFFECT_H

#include "decomp.h"
#include "object.h"
#include "types.h"

// An explosion, fire or other visual effect in the world (g_effects). Each slot keeps the scene
// object for its type once one is created.
// SIZE 0x28
typedef struct Effect {
	SceneObject* m_object; // 0x00
	MechS32 m_position[3]; // 0x04
	MechS32 m_timeLeft;    // 0x10 — ticks
	MechS32 m_active;      // 0x14
	MechS32 m_animation;   // 0x18 — the animation it plays, or -1
	MechS32 m_hasCamera;   // 0x1c — the effect camera follows it
	MechS32 m_type;        // 0x20 — an index into g_effectInfo
	MechS32 m_owner;       // 0x24 — the player it damages on behalf of, or -2
} Effect;

#endif // EFFECT_H
