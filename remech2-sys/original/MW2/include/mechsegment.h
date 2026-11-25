#ifndef MECHSEGMENT_H
#define MECHSEGMENT_H

#include "types.h"

struct SceneObject;

// A saved scene object of a mech (RememberMechSegments): its links, position and rotation,
// restored with the object by RestoreMechSegments.
typedef struct MechSegment {
	struct MechSegment* m_firstChild;
	struct MechSegment* m_nextSibling;
	struct SceneObject* m_obj;
	struct SceneObject* m_parent;
	int32_t m_position[3];
	int32_t m_rotation[3];
} MechSegment;

#endif // MECHSEGMENT_H
