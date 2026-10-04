#ifndef DEBRISPIECE_H
#define DEBRISPIECE_H

#include "types.h"

struct SceneObject;

// A scene object thrown off as wreckage: it flies under gravity, spinning, and bounces on the
// terrain until it comes to rest (UpdateDebrisPiece).
// SIZE 0x24
typedef struct DebrisPiece {
	MechS32 m_unk0x00;         // 0x00
	struct SceneObject* m_obj; // 0x04
	MechS32 m_velocityX;       // 0x08
	MechS32 m_velocityY;       // 0x0c
	MechS32 m_velocityZ;       // 0x10
	MechS32 m_spinX;           // 0x14
	MechS32 m_spinY;           // 0x18
	MechS32 m_spinZ;           // 0x1c
	MechS32 m_acceleration;    // 0x20
} DebrisPiece;

#endif // DEBRISPIECE_H
