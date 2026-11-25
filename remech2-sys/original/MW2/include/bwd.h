#ifndef BWD_H
#define BWD_H

#include "decomp.h"
#include "types.h"

struct BwdStreamKey;

// A node of a BWD stream: a keyword tag (g_bwdTypeCodes) and the node's size, header included.
typedef struct BwdNode {
	uint32_t m_type; // 0x00
	int32_t m_size;  // 0x04
} BwdNode;

// A BWD stream's first node: the stream's size and the largest node size it allows.
typedef struct BwdHeader {
	uint32_t m_type;       // 0x00 — "BWD"
	int32_t m_size;        // 0x04
	int32_t m_maxNodeSize; // 0x08
} BwdHeader;

#pragma pack(1)
// An open BWD stream: a resource read from the resource file, or a file read into the heap.
typedef struct BwdStream {
	int32_t m_fromResource; // 0x00 — 0: m_data is a heap block
	int16_t m_id;           // 0x04 — the BWD resource, or -1 for a file
	char m_name[9];         // 0x06
	int32_t m_size;         // 0x0f
	int32_t m_maxNodeSize;  // 0x13
	void* m_data;           // 0x17
	BwdNode* m_current;     // 0x1b
} BwdStream;
#pragma pack()

// A function run on a BWD stream (ExecuteInclude's callback).
typedef int32_t (*BwdStreamFn)(BwdStream* p_stream);

// The functions and globals of bwd.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_streamsFromFiles;
	extern int32_t g_logStreams;

	BwdStream* OpenBwdStream(struct BwdStreamKey* p_key, BwdStream* p_stream);
	BwdNode* GetNextNode(BwdStream* p_stream);
	void UnloadResource(BwdStream* p_stream);
	char* GetKeywordName(uint32_t p_code);
	void LogDebugLine(char* p_text);
	void LogKeywordName(uint32_t p_code);

#ifdef __cplusplus
}
#endif

#endif // BWD_H
