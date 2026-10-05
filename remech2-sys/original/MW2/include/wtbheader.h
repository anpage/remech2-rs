#ifndef WTBHEADER_H
#define WTBHEADER_H

#include "decomp.h"
#include "types.h"

// The header of a shape record ("WTBO") that LoadShapeRecord reads: m_vertexCount WtbVertex
// entries and m_faceCount WtbFace entries follow it.
// SIZE 0x20
typedef struct WtbHeader {
	MechS32 m_tag;         // 0x00 — "WTBO"
	MechS32 m_checksum;    // 0x04 — of the vertices and faces, modulo 0x100000
	MechChar m_name[16];   // 0x08 — "name_key" picks the level of detail
	MechS16 m_vertexCount; // 0x18
	MechS16 m_faceCount;   // 0x1a
	MechS16 m_flags;       // 0x1c
	undefined2 m_unk0x1e;  // 0x1e
} WtbHeader;

#endif // WTBHEADER_H
