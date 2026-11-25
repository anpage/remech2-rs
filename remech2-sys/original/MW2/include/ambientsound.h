#ifndef AMBIENTSOUND_H
#define AMBIENTSOUND_H

#include "types.h"

struct SceneObject;
struct Shape;

// A sound that loops on an object while the eyepoint is in range (UpdateAmbientSound).
// AmbientSoundTask allocates 0x1e bytes for it, so it is packed.
#pragma pack(2)
typedef struct AmbientSound {
	int32_t m_range;           // 0x00
	int32_t m_slot;            // 0x04 — the sample slot, 8-15, or -1
	void* m_data;              // 0x08 — the loaded resource
	struct Shape** m_shape;    // 0x0c — the star's shape (GetStaticShapeSlot)
	struct SceneObject* m_obj; // 0x10
	int32_t m_enabled;         // 0x14 — 0 stops the task
	int32_t m_skip;            // 0x18 — skips one update
	int16_t m_id;              // 0x1c — the sound resource
} AmbientSound;
#pragma pack()

#endif // AMBIENTSOUND_H
