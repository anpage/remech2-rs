#ifndef MECHSEGMENT_H
#define MECHSEGMENT_H

#include "decomp.h"
#include "types.h"

struct SceneObject;

// A saved scene object of a mech (RememberMechSegments): its links, position and rotation,
// restored with the object by FUN_10080014.
// SIZE 0x28
typedef struct MechSegment {
	struct MechSegment* m_firstChild;  // 0x00
	struct MechSegment* m_nextSibling; // 0x04
	struct SceneObject* m_obj;         // 0x08
	struct SceneObject* m_parent;      // 0x0c
	MechS32 m_position[3];             // 0x10
	undefined4 m_rotation[3];          // 0x1c
} MechSegment;

#endif // MECHSEGMENT_H
