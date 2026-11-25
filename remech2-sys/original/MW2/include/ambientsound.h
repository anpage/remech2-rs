#ifndef AMBIENTSOUND_H
#define AMBIENTSOUND_H

#include "types.h"

struct SceneObject;
struct Shape;

// A sound that loops on an object while the eyepoint is in range (UpdateAmbientSound).
// AmbientSoundTask allocates 0x1e bytes for it, so it is packed.
#pragma pack(2)
typedef struct AmbientSound {
	int32_t m_range;
	int32_t m_slot;         // the sample slot, 8-15, or -1
	void* m_data;           // the loaded resource
	struct Shape** m_shape; // the star's shape (GetStaticShapeSlot)
	struct SceneObject* m_obj;
	int32_t m_enabled; // 0 stops the task
	int32_t m_skip;    // skips one update
	int16_t m_id;      // the sound resource
} AmbientSound;
#pragma pack()

#endif // AMBIENTSOUND_H
