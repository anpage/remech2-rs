#ifndef DEBRISCHUNK_H
#define DEBRISCHUNK_H

#include "object.h"
#include "types.h"

// A scene object blown off a model (BlowOffChunk): it flies as a DebrisPiece until it is shot
// to pieces (DamageChunk) or times out (UpdateDebris), then goes to its callback.
typedef struct DebrisChunk {
	int32_t m_active;
	SceneObject* m_obj;
	int32_t m_startTime;
	int32_t m_health;
	ObjectCallback m_callback;
} DebrisChunk;

#endif // DEBRISCHUNK_H
