#ifndef DEBRISPIECE_H
#define DEBRISPIECE_H

#include "types.h"

struct SceneObject;

// A scene object thrown off as wreckage: it flies under gravity, spinning, and bounces on the
// terrain until it comes to rest (UpdateDebrisPiece).
typedef struct DebrisPiece {
	struct SceneObject* m_obj;
	int32_t m_velocityX;
	int32_t m_velocityY;
	int32_t m_velocityZ;
	int32_t m_spinX;
	int32_t m_spinY;
	int32_t m_spinZ;
	int32_t m_acceleration;
} DebrisPiece;

#endif // DEBRISPIECE_H
