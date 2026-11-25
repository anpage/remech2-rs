#ifndef DEBRISPIECE_H
#define DEBRISPIECE_H

#include "types.h"

struct SceneObject;

// A scene object thrown off as wreckage: it flies under gravity, spinning, and bounces on the
// terrain until it comes to rest (UpdateDebrisPiece).
typedef struct DebrisPiece {
	struct SceneObject* m_obj; // 0x04
	int32_t m_velocityX;       // 0x08
	int32_t m_velocityY;       // 0x0c
	int32_t m_velocityZ;       // 0x10
	int32_t m_spinX;           // 0x14
	int32_t m_spinY;           // 0x18
	int32_t m_spinZ;           // 0x1c
	int32_t m_acceleration;    // 0x20
} DebrisPiece;

#endif // DEBRISPIECE_H
