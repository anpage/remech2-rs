#ifndef PRJFILE_H
#define PRJFILE_H

#include "types.h"

#pragma pack(1)
typedef struct ArchiveEntry {
	char m_name[4];
	uint32_t m_offset;
	uint32_t m_size;
	uint8_t m_reserved0x0c[0x14 - 0x0c];
	uint16_t m_baseOffset;
	uint8_t m_reserved0x16[0x18 - 0x16];
} ArchiveEntry;

typedef struct ArchiveHeader {
	uint8_t m_reserved0x00[0x0c];
	uint16_t m_entryCount;
	ArchiveEntry m_entries[1];
} ArchiveHeader;

typedef struct ArchiveSlotEntry {
	void* m_data;
} ArchiveSlotEntry;

typedef struct ArchiveSlot {
	int32_t m_fd;
	uint16_t m_open;
	ArchiveHeader* m_header;
	ArchiveSlotEntry m_entries[0x20];
	char m_name[0x20];
} ArchiveSlot;
#pragma pack()

// The functions and globals of prjfile.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern ArchiveSlot g_archiveSlots[1];
	extern void* (*g_archiveAlloc)(uint32_t);
	extern void (*g_archiveFree)(void*);

	void SetArchiveAllocator(void* (*p_alloc)(uint32_t), void (*p_free)(void*));
	int32_t OpenArchive(char* p_name, char p_mode);
	int32_t CloseArchive(int32_t p_handle);
	int32_t LoadArchiveEntries(int32_t p_handle);
	int32_t GetArchiveItemSize(int32_t p_handle, char* p_name, uint16_t p_index);
	int32_t ReadArchiveItem(int32_t p_handle, char* p_name, uint16_t p_index, void* p_data);

#ifdef __cplusplus
}
#endif

#endif // PRJFILE_H
