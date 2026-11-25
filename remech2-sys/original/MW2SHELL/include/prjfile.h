#ifndef PRJFILE_H
#define PRJFILE_H

#include "decomp.h"
#include "types.h"

#pragma pack(1)
typedef struct ArchiveEntry {
	char m_name[4];                   // 0x00
	uint32_t m_offset;                // 0x04
	uint32_t m_size;                  // 0x08
	undefined m_unk0x0c[0x14 - 0x0c]; // 0x0c — never accessed
	uint16_t m_baseOffset;            // 0x14
	undefined m_unk0x16[0x18 - 0x16]; // 0x16 — never accessed
} ArchiveEntry;

typedef struct ArchiveHeader {
	undefined m_unk0x00[0x0c]; // 0x00 — never accessed
	uint16_t m_entryCount;     // 0x0c
	ArchiveEntry m_entries[1]; // 0x0e
} ArchiveHeader;

typedef struct ArchiveSlotEntry {
	void* m_data; // 0x00
} ArchiveSlotEntry;

typedef struct ArchiveSlot {
	int32_t m_fd;                     // 0x00
	uint16_t m_open;                  // 0x04
	ArchiveHeader* m_header;          // 0x06
	ArchiveSlotEntry m_entries[0x20]; // 0x2a
	char m_name[0x20];                // 0x12a
} ArchiveSlot;
#pragma pack()

// The functions and globals of prjfile.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern ArchiveSlot g_archiveSlots[1];
	extern void* (*g_archiveAlloc)(undefined4);
	extern void (*g_archiveFree)(void*);

	void SetArchiveAllocator(void* (*p_alloc)(undefined4), void (*p_free)(void*));
	int32_t OpenArchive(char* p_name, char p_mode);
	int32_t CloseArchive(int32_t p_handle);
	int32_t LoadArchiveEntries(int32_t p_handle);
	int32_t GetArchiveItemSize(int32_t p_handle, char* p_name, uint16_t p_index);
	int32_t ReadArchiveItem(int32_t p_handle, char* p_name, uint16_t p_index, void* p_data);

#ifdef __cplusplus
}
#endif

#endif // PRJFILE_H
