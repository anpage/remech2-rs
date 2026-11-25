#ifndef MECHSEGMENT_H
#define MECHSEGMENT_H

#include "types.h"

struct SceneObject;

// A saved scene object of a mech (RememberMechSegments): its links, position and rotation,
// restored with the object by RestoreMechSegments.
typedef struct MechSegment {
	struct MechSegment* m_firstChild;  // 0x00
	struct MechSegment* m_nextSibling; // 0x04
	struct SceneObject* m_obj;         // 0x08
	struct SceneObject* m_parent;      // 0x0c
	int32_t m_position[3];             // 0x10
	int32_t m_rotation[3];             // 0x1c
} MechSegment;

#endif // MECHSEGMENT_H
