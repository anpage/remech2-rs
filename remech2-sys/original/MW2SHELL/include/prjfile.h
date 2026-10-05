#ifndef PRJFILE_H
#define PRJFILE_H

#include "decomp.h"
#include "types.h"

#pragma pack(1)
typedef struct ArchiveEntry {
	MechChar m_name[4];               // 0x00
	MechU32 m_offset;                 // 0x04
	MechU32 m_size;                   // 0x08
	undefined m_unk0x0c[0x14 - 0x0c]; // 0x0c — never accessed
	MechU16 m_baseOffset;             // 0x14
	undefined m_unk0x16[0x18 - 0x16]; // 0x16 — never accessed
} ArchiveEntry;

typedef struct ArchiveHeader {
	undefined m_unk0x00[0x0c]; // 0x00 — never accessed
	MechU16 m_entryCount;      // 0x0c
	ArchiveEntry m_entries[1]; // 0x0e
} ArchiveHeader;

typedef struct ArchiveSlotEntry {
	void* m_data;    // 0x00
	void* m_unk0x04; // 0x04 — only freed and cleared by CloseArchive; nothing else stores to it
} ArchiveSlotEntry;

typedef struct ArchiveSlot {
	MechS32 m_fd;                     // 0x00
	MechU16 m_open;                   // 0x04
	ArchiveHeader* m_header;          // 0x06
	undefined m_unk0x0a[0x2a - 0x0a]; // 0x0a — never accessed
	ArchiveSlotEntry m_entries[0x20]; // 0x2a
	MechChar m_name[0x20];            // 0x12a
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
	MechS32 OpenArchive(char* p_name, MechChar p_mode);
	MechS32 CloseArchive(MechS32 p_handle);
	MechS32 LoadArchiveEntries(MechS32 p_handle);
	MechS32 GetArchiveItemSize(MechS32 p_handle, MechChar* p_name, MechU16 p_index);
	MechS32 ReadArchiveItem(MechS32 p_handle, MechChar* p_name, MechU16 p_index, void* p_data);

#ifdef __cplusplus
}
#endif

#endif // PRJFILE_H
