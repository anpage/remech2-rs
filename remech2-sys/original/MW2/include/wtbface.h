#ifndef WTBFACE_H
#define WTBFACE_H

#include "types.h"

// A face of a shape record (WtbHeader): 0x0c bytes with up to four vertex indices, 0x12 with
// more.
typedef struct WtbFace {
	uint16_t m_id; // MapFaceId maps it
	uint16_t m_count;
	uint16_t m_indices[4];
} WtbFace;

#endif // WTBFACE_H
