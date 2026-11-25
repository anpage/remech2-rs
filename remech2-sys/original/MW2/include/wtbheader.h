#ifndef WTBHEADER_H
#define WTBHEADER_H

#include "types.h"

// The header of a shape record ("WTBO") that LoadShapeRecord reads: m_vertexCount WtbVertex
// entries and m_faceCount WtbFace entries follow it.
typedef struct WtbHeader {
	int32_t m_tag;      // "WTBO"
	int32_t m_checksum; // of the vertices and faces, modulo 0x100000
	char m_name[16];    // "name_key" picks the level of detail
	int16_t m_vertexCount;
	int16_t m_faceCount;
	int16_t m_flags;
	uint16_t m_reserved0x1e;
} WtbHeader;

#endif // WTBHEADER_H
