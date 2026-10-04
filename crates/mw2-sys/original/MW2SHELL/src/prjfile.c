#include "prjfile.h"

#include "decomp.h"
#include "files.h"
#include "types.h"

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>

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

DECOMP_SIZE_ASSERT(ArchiveEntry, 0x18)
DECOMP_SIZE_ASSERT(ArchiveSlot, 0x14a)

// One slot: the dispdib globals follow in the original.
// GLOBAL: MW2SHELL 0x10096610
ArchiveSlot g_archiveSlots[1];

// The heap callbacks the archive unit allocates through, registered by SetArchiveAllocator
// (ProjectArchive passes PrjHeapAlloc and PrjHeapFree).
// GLOBAL: MW2SHELL 0x10066da0
void* (*g_archiveAlloc)(undefined4) = NULL;

// GLOBAL: MW2SHELL 0x10066da4
void (*g_archiveFree)(void*) = NULL;

// FUNCTION: MW2SHELL 0x1002fb90
void SetArchiveAllocator(void* (*p_alloc)(undefined4), void (*p_free)(void*))
{
	g_archiveAlloc = p_alloc;
	g_archiveFree = p_free;
}

// FUNCTION: MW2SHELL 0x1002fbab
void* ArchiveAlloc(undefined4 p_size)
{
	return g_archiveAlloc(p_size);
}

// FUNCTION: MW2SHELL 0x1002fbc8
void ArchiveFree(void* p_block)
{
	g_archiveFree(p_block);
}

// Writes p_size bytes in chunks of at most 0x4000. Returns p_size, or -1 on a short write.
// Stack-slot permutation: remaining and chunk swap homes.
// FUNCTION: MW2SHELL 0x1002fbe0
MechS32 ArchiveWrite(MechS32 p_fd, MechU8* p_buffer, MechU32 p_size)
{
	MechU8* buffer;
	MechU32 remaining;
	MechU32 chunk;

	remaining = p_size;
	buffer = p_buffer;
	while (remaining > 0) {
		chunk = remaining < 0x4000 ? remaining : 0x4000;
		if (_write(p_fd, buffer, chunk) != chunk) {
			return -1;
		}

		remaining -= chunk;
		buffer += chunk;
	}

	return p_size;
}

// Reads p_size bytes in chunks of at most 0x4000. Returns p_size, or -1 on a short read.
// Stack-slot permutation: remaining and chunk swap homes.
// FUNCTION: MW2SHELL 0x1002fc5e
MechS32 ArchiveRead(MechS32 p_fd, MechU8* p_buffer, MechU32 p_size)
{
	MechU8* buffer;
	MechU32 remaining;
	MechU32 chunk;

	remaining = p_size;
	buffer = p_buffer;
	while (remaining > 0) {
		chunk = remaining < 0x4000 ? remaining : 0x4000;
		if (_read(p_fd, buffer, chunk) != chunk) {
			return -1;
		}

		remaining -= chunk;
		buffer += chunk;
	}

	return p_size;
}

// Opens the project archive p_name in a free slot: p_mode 2 for reading and writing (a project
// marked 0xfe returns -2), 0 for reading. Loads its header and returns the slot, or -1.
// Not 100%: the stack slots of the locals are permuted.
// FUNCTION: MW2SHELL 0x1002fcdc
MechS32 OpenArchive(char* p_name, MechChar p_mode)
{
	ArchiveHeader* header;
	MechU16 slot;
	MechU32 size;
	MechS32 fd;
	MechU8 tag[12];

	for (slot = 0; slot < 1 && g_archiveSlots[slot].m_open; slot++) {
	}

	if (slot >= 1) {
		return -1;
	}

	if (p_mode == 2) {
		fd = MechOpen(p_name, c_mechOpenReadWrite);
		if (fd == -1) {
			return -1;
		}
		else {
			_read(fd, tag, sizeof(tag));
			if (_strnicmp((char*) tag, "PROJ", 4)) {
				return -1;
			}

			if (tag[8] == 0xfe) {
				_close(fd);
				return -2;
			}
		}
	}
	else if (p_mode == 0) {
		fd = MechOpen(p_name, c_mechOpenRead);
		if (fd == -1) {
			return -1;
		}
		else {
			_read(fd, tag, sizeof(tag));
			if (_strnicmp((char*) tag, "PROJ", 4)) {
				return -1;
			}
		}
	}
	else {
		return -1;
	}

	_lseek(fd, 0x10, SEEK_SET);
	if (ArchiveRead(fd, (MechU8*) &size, 4) != 4) {
		return -1;
	}

	size += 8;
	header = (ArchiveHeader*) ArchiveAlloc(size);
	if (header == NULL) {
		return -1;
	}

	_lseek(fd, 0xc, SEEK_SET);
	if (ArchiveRead(fd, (MechU8*) header, size) != size) {
		ArchiveFree(header);
		return -1;
	}

	g_archiveSlots[slot].m_fd = fd;
	g_archiveSlots[slot].m_header = header;
	g_archiveSlots[slot].m_open = 1;
	_strnset(g_archiveSlots[slot].m_name, 0, sizeof(g_archiveSlots[slot].m_name));
	strncpy(g_archiveSlots[slot].m_name, p_name, sizeof(g_archiveSlots[slot].m_name));
	return slot;
}

// Release the header and cached archive entries, then close its file descriptor.
// FUNCTION: MW2SHELL 0x1002ffc9
MechS32 CloseArchive(MechS32 p_handle)
{
	MechU16 index;

	ArchiveFree(g_archiveSlots[p_handle].m_header);
	g_archiveSlots[p_handle].m_header = NULL;
	for (index = 0; index < 0x20; index++) {
		if (g_archiveSlots[p_handle].m_entries[index].m_data != NULL) {
			ArchiveFree(g_archiveSlots[p_handle].m_entries[index].m_data);
			g_archiveSlots[p_handle].m_entries[index].m_data = NULL;
		}
		if (g_archiveSlots[p_handle].m_entries[index].m_unk0x04 != NULL) {
			ArchiveFree(g_archiveSlots[p_handle].m_entries[index].m_unk0x04);
			g_archiveSlots[p_handle].m_entries[index].m_unk0x04 = NULL;
		}
	}

	g_archiveSlots[p_handle].m_open = 0;
	_strnset(g_archiveSlots[p_handle].m_name, 0, 0x20);
	return _close(g_archiveSlots[p_handle].m_fd);
}

// Find an archive entry by its four-byte type tag. Returns 0xffff if absent.
// Stack-slot permutation: count and entries.
// FUNCTION: MW2SHELL 0x10030177
MechU16 FindArchiveEntry(MechS32 p_handle, MechChar* p_name)
{
	MechChar found;
	MechU16 index;
	MechU16 count;
	ArchiveEntry* entries;

	found = 0;
	count = g_archiveSlots[p_handle].m_header->m_entryCount;
	entries = g_archiveSlots[p_handle].m_header->m_entries;
	index = 0;
	while ((count > index) & !found) {
		if (strncmp(entries[index].m_name, p_name, 4) == 0) {
			found = 1;
		}
		else {
			index++;
		}
	}

	if (found) {
		return index;
	}
	else {
		return 0xffff;
	}
}

// Read and cache every archive entry whose on-disk data offset is nonzero.
// Stack-slot permutation: count, index and size.
// FUNCTION: MW2SHELL 0x1003024f
MechS32 LoadArchiveEntries(MechS32 p_handle)
{
	MechU16 count;
	ArchiveEntry* entries;
	MechU16 index;
	MechU32 size;
	void* data;

	count = g_archiveSlots[p_handle].m_header->m_entryCount;
	entries = g_archiveSlots[p_handle].m_header->m_entries;
	for (index = 0; index < count; index++) {
		if (entries[index].m_offset != 0) {
			size = entries[index].m_size;
			data = ArchiveAlloc(size);
			if (data == NULL) {
				return -1;
			}

			_lseek(g_archiveSlots[p_handle].m_fd, entries[index].m_offset, SEEK_SET);
			if (ArchiveRead(g_archiveSlots[p_handle].m_fd, (MechU8*) data, size) != size) {
				ArchiveFree(data);
				return -1;
			}

			g_archiveSlots[p_handle].m_entries[index].m_data = data;
		}
	}

	return 0;
}

// Compute a cached subresource length, clamping non-positive lengths to zero.
// Stack-slot permutation: offsets, entries and length.
// FUNCTION: MW2SHELL 0x100303b5
MechS32 GetArchiveItemSize(MechS32 p_handle, MechChar* p_name, MechU16 p_index)
{
	MechS16 entry;
	MechU16 index;
	MechS32* offsets;
	ArchiveEntry* entries;
	MechS32 length;

	if (p_index == 0) {
		return -1;
	}

	entry = FindArchiveEntry(p_handle, p_name);
	if (entry >= 0) {
		if (g_archiveSlots[p_handle].m_entries[entry].m_data == NULL && LoadArchiveEntries(p_handle) == -1) {
			return -1;
		}

		index = p_index;
		offsets = (MechS32*) ((MechU8*) g_archiveSlots[p_handle].m_entries[entry].m_data + 0x16);
		entries = g_archiveSlots[p_handle].m_header->m_entries;
		length = offsets[index * 2 + 1] - entries[entry].m_baseOffset;
		return length > 0 ? length : 0;
	}
	else {
		return -1;
	}
}

// Look up a subresource's byte offset and length in the cached archive entry.
// Stack-slot permutation: offsets and entries.
// FUNCTION: MW2SHELL 0x100304c5
MechS32 SeekArchiveItem(MechS32 p_handle, MechChar* p_name, MechU32 p_index, void** p_offset, MechS32* p_size)
{
	MechU16 entry;
	ArchiveEntry* entries;
	MechU8* offsets;
	MechS32 offset;

	entry = FindArchiveEntry(p_handle, p_name);
	if ((MechS32) entry == -1) {
		return -1;
	}
	if (g_archiveSlots[p_handle].m_entries[entry].m_data == NULL && LoadArchiveEntries(p_handle) == -1) {
		return -1;
	}

	offsets = (MechU8*) g_archiveSlots[p_handle].m_entries[entry].m_data + 0x16;
	entries = g_archiveSlots[p_handle].m_header->m_entries;
	offset = entries[entry].m_baseOffset + *(MechU32*) (offsets + (p_index & 0xffff) * 8);
	if (p_offset != NULL) {
		*p_offset = MECH_S32_TO_PTR(offset);
	}
	if (p_size != NULL) {
		*p_size = *(MechS32*) (offsets + (p_index & 0xffff) * 8 + 4) - entries[entry].m_baseOffset;
	}

	_lseek(g_archiveSlots[p_handle].m_fd, offset, SEEK_SET);
	return 0;
}

// Read p_size bytes after selecting the archive descriptor and offset.
// FUNCTION: MW2SHELL 0x10030620
void ReadArchiveAt(void* p_data, MechS32 p_handle, void* p_offset, MechU32 p_size)
{
	p_handle = g_archiveSlots[p_handle].m_fd;
	_lseek(p_handle, MECH_PTR_TO_S32(p_offset), SEEK_SET);
	ArchiveRead(p_handle, (MechU8*) p_data, p_size);
	return;
}

// Read a cached archive subresource into the caller's buffer.
// Stack-slot permutation: entry, offsets, entries, offset and size.
// FUNCTION: MW2SHELL 0x1003066e
MechS32 ReadArchiveItem(MechS32 p_handle, MechChar* p_name, MechU16 p_index, void* p_data)
{
	MechS16 entry;
	MechU16 index;
	ArchiveEntry* entries;
	MechU8* offsets;
	MechU32 size;
	MechS32 offset;

	entry = FindArchiveEntry(p_handle, p_name);
	if (entry != -1) {
		if (g_archiveSlots[p_handle].m_entries[entry].m_data == NULL && LoadArchiveEntries(p_handle) == -1) {
			return -1;
		}

		index = p_index;
		offsets = (MechU8*) g_archiveSlots[p_handle].m_entries[entry].m_data + 0x16;
		entries = g_archiveSlots[p_handle].m_header->m_entries;
		offset = entries[entry].m_baseOffset + *(MechU32*) (offsets + index * 8);
		size = *(MechU32*) (offsets + index * 8 + 4) - entries[entry].m_baseOffset;
		if (_lseek(g_archiveSlots[p_handle].m_fd, offset, SEEK_SET) == -1) {
			return -1;
		}

		if (ArchiveRead(g_archiveSlots[p_handle].m_fd, (MechU8*) p_data, size) != size) {
			return -1;
		}
		else {
			return 0;
		}
	}
	else {
		return -1;
	}
}

// Return the archive offset and descriptor of a cached subresource.
// Stack-slot permutation: offsets and entries.
// FUNCTION: MW2SHELL 0x100307f4
MechS32 GetArchiveItemOffset(MechS32 p_handle, MechChar* p_name, MechU16 p_index, MechS32* p_fd)
{
	MechS16 entry;
	MechU16 index;
	MechU8* offsets;
	ArchiveEntry* entries;
	MechS32 offset;

	entry = FindArchiveEntry(p_handle, p_name);
	if (entry != -1) {
		if (g_archiveSlots[p_handle].m_entries[entry].m_data == NULL && LoadArchiveEntries(p_handle) == -1) {
			return 0;
		}

		index = p_index;
		offsets = (MechU8*) g_archiveSlots[p_handle].m_entries[entry].m_data + 0x16;
		entries = g_archiveSlots[p_handle].m_header->m_entries;
		offset = entries[entry].m_baseOffset + *(MechU32*) (offsets + index * 8);
		*p_fd = g_archiveSlots[p_handle].m_fd;
		return offset;
	}
	else {
		return 0;
	}
}
