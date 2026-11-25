#ifndef PRJFILE_H
#define PRJFILE_H

#include "types.h"

#pragma pack(push, 1)

// A resource type in a PROJ file's header: its tag and its index of resources.
typedef struct PrjType {
	char m_tag[4];
	int32_t m_indexOffset; // in the file, 0: no index
	uint32_t m_indexSize;
	uint8_t m_reserved0x0c[0x14 - 0x0c];
	uint16_t m_indexBase; // a base the index's entries are relative to
	uint16_t m_reserved0x16;
} PrjType;

// The header of a PROJ file, from file offset 0x0c.
typedef struct PrjHeader {
	uint8_t m_reserved0x00[0x0c];
	uint16_t m_typeCount;
	PrjType m_types[1];
} PrjHeader;

typedef struct PrjIndexEntry {
	int32_t m_offset; // from the type's base
	int32_t m_end;    // from the type's base
} PrjIndexEntry;

// A type's index of resources, by resource ID.
typedef struct PrjIndex {
	uint8_t m_reserved0x00[0x16];
	PrjIndexEntry m_entries[1];
} PrjIndex;

typedef struct PrjTypeSlot {
	PrjIndex* m_index; // loaded on first use
} PrjTypeSlot;

// An open PROJ file.
typedef struct PrjFile {
	int32_t m_fd;
	uint16_t m_open;
	PrjHeader* m_header;
	PrjTypeSlot m_types[32];
	char m_name[0x20];
} PrjFile;

#pragma pack(pop)

typedef void* (*PrjAllocFn)(uint32_t p_size);
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
	void* PrjAlloc(uint32_t p_size);
	void PrjFreeBlock(void* p_block);
	int32_t WritePrjBytes(int32_t p_fd, void* p_buffer, uint32_t p_length);
	int32_t ReadPrjBytes(int32_t p_fd, void* p_buffer, uint32_t p_length);
	int32_t OpenPrjFile(const char* p_name, char p_mode);
	int32_t ClosePrjFile(int32_t p_file);
	uint16_t FindPrjType(int32_t p_file, const char* p_type);
	int32_t LoadPrjIndexes(int32_t p_file);
	int32_t GetPrjResourceSize(int32_t p_file, const char* p_type, uint16_t p_id);
	int32_t SeekPrjResource(int32_t p_file, const char* p_type, uint16_t p_id, int32_t* p_offset, int32_t* p_size);
	int32_t ReadPrjAt(void* p_buffer, int32_t p_file, int32_t p_offset, uint32_t p_length);
	int32_t ReadPrjResource(int32_t p_file, const char* p_type, uint16_t p_id, void* p_buffer);
	int32_t GetPrjResourceOffset(int32_t p_file, const char* p_type, uint16_t p_id, int32_t* p_fd);

#ifdef __cplusplus
}
#endif

#endif // PRJFILE_H
