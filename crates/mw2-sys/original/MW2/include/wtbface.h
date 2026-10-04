#ifndef WTBFACE_H
#define WTBFACE_H

#include "decomp.h"
#include "types.h"

// A face of a shape record (WtbHeader): 0x0c bytes with up to four vertex indices, 0x12 with
// more.
typedef struct WtbFace {
	MechU16 m_id;         // 0x00 — MapFaceId maps it
	MechU16 m_count;      // 0x02
	MechU16 m_indices[4]; // 0x04
} WtbFace;

#endif // WTBFACE_H
