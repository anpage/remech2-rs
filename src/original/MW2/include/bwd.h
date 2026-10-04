#ifndef BWD_H
#define BWD_H

#include "decomp.h"
#include "types.h"

struct BwdStreamKey;

// A node of a BWD stream: a keyword tag (g_bwdTypeCodes) and the node's size, header included.
// SIZE 0x08
typedef struct BwdNode {
	MechU32 m_type; // 0x00
	MechS32 m_size; // 0x04
} BwdNode;

// A BWD stream's first node: the stream's size and the largest node size it allows.
// SIZE 0x0c
typedef struct BwdHeader {
	MechU32 m_type;        // 0x00 — "BWD"
	MechS32 m_size;        // 0x04
	MechS32 m_maxNodeSize; // 0x08
} BwdHeader;

#pragma pack(1)
// An open BWD stream: a resource read from the resource file, or a file read into the heap.
// SIZE 0x1f
typedef struct BwdStream {
	MechS32 m_fromResource; // 0x00 — 0: m_data is a heap block
	MechS16 m_id;           // 0x04 — the BWD resource, or -1 for a file
	MechChar m_name[9];     // 0x06
	MechS32 m_size;         // 0x0f
	MechS32 m_maxNodeSize;  // 0x13
	void* m_data;           // 0x17
	BwdNode* m_current;     // 0x1b
} BwdStream;
#pragma pack()

// A function run on a BWD stream (ExecuteInclude's callback).
typedef MechS32 (*BwdStreamFn)(BwdStream* p_stream);

// The functions and globals of bwd.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_streamsFromFiles;
	extern MechS32 g_logStreams;

	BwdStream* OpenBwdStream(struct BwdStreamKey* p_key, BwdStream* p_stream);
	BwdNode* GetNextNode(BwdStream* p_stream);
	void UnloadResource(BwdStream* p_stream);
	MechChar* GetKeywordName(MechU32 p_code);
	void LogDebugLine(MechChar* p_text);
	void LogKeywordName(MechU32 p_code);

#ifdef __cplusplus
}
#endif

#endif // BWD_H
