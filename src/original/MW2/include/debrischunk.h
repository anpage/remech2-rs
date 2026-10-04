#ifndef DEBRISCHUNK_H
#define DEBRISCHUNK_H

#include "object.h"
#include "types.h"

// A scene object blown off a model (BlowOffChunk): it flies as a DebrisPiece until it is shot
// to pieces (DamageChunk) or times out (UpdateDebris), then goes to its callback.
// SIZE 0x14
typedef struct DebrisChunk {
	MechS32 m_active;          // 0x00
	SceneObject* m_obj;        // 0x04
	MechS32 m_startTime;       // 0x08
	MechS32 m_health;          // 0x0c
	ObjectCallback m_callback; // 0x10
} DebrisChunk;

#endif // DEBRISCHUNK_H
