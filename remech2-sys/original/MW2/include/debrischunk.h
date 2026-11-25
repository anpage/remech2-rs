#ifndef DEBRISCHUNK_H
#define DEBRISCHUNK_H

#include "object.h"
#include "types.h"

// A scene object blown off a model (BlowOffChunk): it flies as a DebrisPiece until it is shot
// to pieces (DamageChunk) or times out (UpdateDebris), then goes to its callback.
typedef struct DebrisChunk {
	int32_t m_active;          // 0x00
	SceneObject* m_obj;        // 0x04
	int32_t m_startTime;       // 0x08
	int32_t m_health;          // 0x0c
	ObjectCallback m_callback; // 0x10
} DebrisChunk;

#endif // DEBRISCHUNK_H
