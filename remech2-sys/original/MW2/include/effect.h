#ifndef EFFECT_H
#define EFFECT_H

#include "object.h"
#include "types.h"

// An explosion, fire or other visual effect in the world (g_effects). Each slot keeps the scene
// object for its type once one is created.
typedef struct Effect {
	SceneObject* m_object;
	int32_t m_position[3];
	int32_t m_timeLeft; // ticks
	int32_t m_active;
	int32_t m_animation; // the animation it plays, or -1
	int32_t m_hasCamera; // the effect camera follows it
	int32_t m_type;      // an index into g_effectInfo
	int32_t m_owner;     // the player it damages on behalf of, or -2
} Effect;

#endif // EFFECT_H
