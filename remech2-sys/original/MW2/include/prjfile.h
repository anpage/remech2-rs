#ifndef PRJFILE_H
#define PRJFILE_H

#include "decomp.h"
#include "types.h"

#pragma pack(push, 1)

// A resource type in a PROJ file's header: its tag and its index of resources.
// SIZE 0x18
typedef struct PrjType {
	MechChar m_tag[4];                // 0x00
	MechS32 m_indexOffset;            // 0x04 — in the file, 0: no index
	MechU32 m_indexSize;              // 0x08
	undefined m_unk0x0c[0x14 - 0x0c]; // 0x0c
	MechU16 m_indexBase;              // 0x14 — a base the index's entries are relative to
	undefined2 m_unk0x16;             // 0x16
} PrjType;

// The header of a PROJ file, from file offset 0x0c.
typedef struct PrjHeader {
	undefined m_unk0x00[0x0c]; // 0x00
	MechU16 m_typeCount;       // 0x0c
	PrjType m_types[1];        // 0x0e
} PrjHeader;

// SIZE 0x08
typedef struct PrjIndexEntry {
	MechS32 m_offset; // 0x00 — from the type's base
	MechS32 m_end;    // 0x04 — from the type's base
} PrjIndexEntry;

// A type's index of resources, by resource ID.
typedef struct PrjIndex {
	undefined m_unk0x00[0x16];  // 0x00
	PrjIndexEntry m_entries[1]; // 0x16
} PrjIndex;

// SIZE 0x08
typedef struct PrjTypeSlot {
	PrjIndex* m_index; // 0x00 — loaded on first use
	void* m_unk0x04;   // 0x04
} PrjTypeSlot;

// An open PROJ file.
// SIZE 0x14a
typedef struct PrjFile {
	MechS32 m_fd;                     // 0x00
	MechU16 m_open;                   // 0x04
	PrjHeader* m_header;              // 0x06
	undefined m_unk0x0a[0x2a - 0x0a]; // 0x0a
	PrjTypeSlot m_types[32];          // 0x2a
	MechChar m_name[0x20];            // 0x12a
} PrjFile;

#pragma pack(pop)

typedef void* (*PrjAllocFn)(MechU32 p_size);
typedef void (*PrjFreeFn)(void* p_block);

// The functions and globals of prjfile.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern PrjAllocFn g_prjAlloc;
	extern PrjFreeFn g_prjFree;
	extern PrjFile g_prjFiles[1];

	void SetPrjAllocator(PrjAllocFn p_alloc, PrjFreeFn p_free);
	void* PrjAlloc(MechU32 p_size);
	void PrjFreeBlock(void* p_block);
	MechS32 WritePrjBytes(MechS32 p_fd, void* p_buffer, MechU32 p_length);
	MechS32 ReadPrjBytes(MechS32 p_fd, void* p_buffer, MechU32 p_length);
	MechS32 OpenPrjFile(const MechChar* p_name, MechChar p_mode);
	MechS32 ClosePrjFile(MechS32 p_file);
	MechU16 FindPrjType(MechS32 p_file, const MechChar* p_type);
	MechS32 LoadPrjIndexes(MechS32 p_file);
	MechS32 GetPrjResourceSize(MechS32 p_file, const MechChar* p_type, MechU16 p_id);
	MechS32 SeekPrjResource(MechS32 p_file, const MechChar* p_type, MechU16 p_id, MechS32* p_offset, MechS32* p_size);
	MechS32 ReadPrjAt(void* p_buffer, MechS32 p_file, MechS32 p_offset, MechU32 p_length);
	MechS32 ReadPrjResource(MechS32 p_file, const MechChar* p_type, MechU16 p_id, void* p_buffer);
	MechS32 GetPrjResourceOffset(MechS32 p_file, const MechChar* p_type, MechU16 p_id, MechS32* p_fd);

#ifdef __cplusplus
}
#endif

#endif // PRJFILE_H
