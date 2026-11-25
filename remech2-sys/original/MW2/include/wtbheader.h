#ifndef WTBHEADER_H
#define WTBHEADER_H

#include "types.h"

// The header of a shape record ("WTBO") that LoadShapeRecord reads: m_vertexCount WtbVertex
// entries and m_faceCount WtbFace entries follow it.
typedef struct WtbHeader {
	int32_t m_tag;         // 0x00 — "WTBO"
	int32_t m_checksum;    // 0x04 — of the vertices and faces, modulo 0x100000
	char m_name[16];       // 0x08 — "name_key" picks the level of detail
	int16_t m_vertexCount; // 0x18
	int16_t m_faceCount;   // 0x1a
	int16_t m_flags;       // 0x1c
	uint16_t m_unk0x1e;    // 0x1e
} WtbHeader;

#endif // WTBHEADER_H
