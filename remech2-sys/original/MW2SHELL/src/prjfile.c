#include "prjfile.h"

#include "decomp.h"
#include "files.h"
#include "types.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

// One slot: the dispdib globals follow in the original.
ArchiveSlot g_archiveSlots[1];

// The heap callbacks the archive unit allocates through, registered by SetArchiveAllocator
// (ProjectArchive passes PrjHeapAlloc and PrjHeapFree).
void* (*g_archiveAlloc)(undefined4) = NULL;

void (*g_archiveFree)(void*) = NULL;

void SetArchiveAllocator(void* (*p_alloc)(undefined4), void (*p_free)(void*))
{
	g_archiveAlloc = p_alloc;
	g_archiveFree = p_free;
}

void* ArchiveAlloc(undefined4 p_size)
{
	return g_archiveAlloc(p_size);
}

void ArchiveFree(void* p_block)
{
	g_archiveFree(p_block);
}

// Writes p_size bytes in chunks of at most 0x4000. Returns p_size, or -1 on a short write.
int32_t ArchiveWrite(int32_t p_fd, uint8_t* p_buffer, uint32_t p_size)
{
	uint8_t* buffer;
	uint32_t remaining;
	uint32_t chunk;

	remaining = p_size;
	buffer = p_buffer;
	while (remaining > 0) {
		chunk = remaining < 0x4000 ? remaining : 0x4000;
		if (MechWrite(p_fd, buffer, chunk) != chunk) {
			return -1;
		}

		remaining -= chunk;
		buffer += chunk;
	}

	return p_size;
}

// Reads p_size bytes in chunks of at most 0x4000. Returns p_size, or -1 on a short read.
int32_t ArchiveRead(int32_t p_fd, uint8_t* p_buffer, uint32_t p_size)
{
	uint8_t* buffer;
	uint32_t remaining;
	uint32_t chunk;

	remaining = p_size;
	buffer = p_buffer;
	while (remaining > 0) {
		chunk = remaining < 0x4000 ? remaining : 0x4000;
		if (MechRead(p_fd, buffer, chunk) != chunk) {
			return -1;
		}

		remaining -= chunk;
		buffer += chunk;
	}

	return p_size;
}

// Opens the project archive p_name in a free slot: p_mode 2 for reading and writing (a project
// marked 0xfe returns -2), 0 for reading. Loads its header and returns the slot, or -1.
int32_t OpenArchive(char* p_name, char p_mode)
{
	ArchiveHeader* header;
	uint16_t slot;
	uint32_t size;
	int32_t fd;
	uint8_t tag[12];

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
			MechRead(fd, tag, sizeof(tag));
			if (strncasecmp((char*) tag, "PROJ", 4)) {
				return -1;
			}

			if (tag[8] == 0xfe) {
				MechClose(fd);
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
			MechRead(fd, tag, sizeof(tag));
			if (strncasecmp((char*) tag, "PROJ", 4)) {
				return -1;
			}
		}
	}
	else {
		return -1;
	}

	MechSeek(fd, 0x10, SEEK_SET);
	if (ArchiveRead(fd, (uint8_t*) &size, 4) != 4) {
		return -1;
	}

	size += 8;
	header = (ArchiveHeader*) ArchiveAlloc(size);
	if (header == NULL) {
		return -1;
	}

	MechSeek(fd, 0xc, SEEK_SET);
	if (ArchiveRead(fd, (uint8_t*) header, size) != size) {
		ArchiveFree(header);
		return -1;
	}

	g_archiveSlots[slot].m_fd = fd;
	g_archiveSlots[slot].m_header = header;
	g_archiveSlots[slot].m_open = 1;
	memset(g_archiveSlots[slot].m_name, 0, sizeof(g_archiveSlots[slot].m_name));
	strncpy(g_archiveSlots[slot].m_name, p_name, sizeof(g_archiveSlots[slot].m_name));
	return slot;
}

// Release the header and cached archive entries, then close its file descriptor.
int32_t CloseArchive(int32_t p_handle)
{
	uint16_t index;

	ArchiveFree(g_archiveSlots[p_handle].m_header);
	g_archiveSlots[p_handle].m_header = NULL;
	for (index = 0; index < 0x20; index++) {
		if (g_archiveSlots[p_handle].m_entries[index].m_data != NULL) {
			ArchiveFree(g_archiveSlots[p_handle].m_entries[index].m_data);
			g_archiveSlots[p_handle].m_entries[index].m_data = NULL;
		}
	}

	g_archiveSlots[p_handle].m_open = 0;
	memset(g_archiveSlots[p_handle].m_name, 0, 0x20);
	return MechClose(g_archiveSlots[p_handle].m_fd);
}

// Find an archive entry by its four-byte type tag. Returns 0xffff if absent.
uint16_t FindArchiveEntry(int32_t p_handle, char* p_name)
{
	char found;
	uint16_t index;
	uint16_t count;
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
int32_t LoadArchiveEntries(int32_t p_handle)
{
	uint16_t count;
	ArchiveEntry* entries;
	uint16_t index;
	uint32_t size;
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

			MechSeek(g_archiveSlots[p_handle].m_fd, entries[index].m_offset, SEEK_SET);
			if (ArchiveRead(g_archiveSlots[p_handle].m_fd, (uint8_t*) data, size) != size) {
				ArchiveFree(data);
				return -1;
			}

			g_archiveSlots[p_handle].m_entries[index].m_data = data;
		}
	}

	return 0;
}

// Compute a cached subresource length, clamping non-positive lengths to zero.
int32_t GetArchiveItemSize(int32_t p_handle, char* p_name, uint16_t p_index)
{
	int16_t entry;
	uint16_t index;
	int32_t* offsets;
	ArchiveEntry* entries;
	int32_t length;

	if (p_index == 0) {
		return -1;
	}

	entry = FindArchiveEntry(p_handle, p_name);
	if (entry >= 0) {
		if (g_archiveSlots[p_handle].m_entries[entry].m_data == NULL && LoadArchiveEntries(p_handle) == -1) {
			return -1;
		}

		index = p_index;
		offsets = (int32_t*) ((uint8_t*) g_archiveSlots[p_handle].m_entries[entry].m_data + 0x16);
		entries = g_archiveSlots[p_handle].m_header->m_entries;
		length = offsets[index * 2 + 1] - entries[entry].m_baseOffset;
		return length > 0 ? length : 0;
	}
	else {
		return -1;
	}
}

// Look up a subresource's byte offset and length in the cached archive entry.
int32_t SeekArchiveItem(int32_t p_handle, char* p_name, uint32_t p_index, void** p_offset, int32_t* p_size)
{
	uint16_t entry;
	ArchiveEntry* entries;
	uint8_t* offsets;
	int32_t offset;

	entry = FindArchiveEntry(p_handle, p_name);
	// FindArchiveEntry's -1 as the 16 bits it's kept in: compared with an int, as the
	// original's source has it, it never matches
	if (entry == (uint16_t) -1) {
		return -1;
	}
	if (g_archiveSlots[p_handle].m_entries[entry].m_data == NULL && LoadArchiveEntries(p_handle) == -1) {
		return -1;
	}

	offsets = (uint8_t*) g_archiveSlots[p_handle].m_entries[entry].m_data + 0x16;
	entries = g_archiveSlots[p_handle].m_header->m_entries;
	offset = entries[entry].m_baseOffset + *(uint32_t*) (offsets + (p_index & 0xffff) * 8);
	if (p_offset != NULL) {
		*p_offset = MECH_S32_TO_PTR(offset);
	}
	if (p_size != NULL) {
		*p_size = *(int32_t*) (offsets + (p_index & 0xffff) * 8 + 4) - entries[entry].m_baseOffset;
	}

	MechSeek(g_archiveSlots[p_handle].m_fd, offset, SEEK_SET);
	return 0;
}

// Read p_size bytes after selecting the archive descriptor and offset.
void ReadArchiveAt(void* p_data, int32_t p_handle, void* p_offset, uint32_t p_size)
{
	p_handle = g_archiveSlots[p_handle].m_fd;
	MechSeek(p_handle, MECH_PTR_TO_S32(p_offset), SEEK_SET);
	ArchiveRead(p_handle, (uint8_t*) p_data, p_size);
}

// Read a cached archive subresource into the caller's buffer.
int32_t ReadArchiveItem(int32_t p_handle, char* p_name, uint16_t p_index, void* p_data)
{
	int16_t entry;
	uint16_t index;
	ArchiveEntry* entries;
	uint8_t* offsets;
	uint32_t size;
	int32_t offset;

	entry = FindArchiveEntry(p_handle, p_name);
	if (entry != -1) {
		if (g_archiveSlots[p_handle].m_entries[entry].m_data == NULL && LoadArchiveEntries(p_handle) == -1) {
			return -1;
		}

		index = p_index;
		offsets = (uint8_t*) g_archiveSlots[p_handle].m_entries[entry].m_data + 0x16;
		entries = g_archiveSlots[p_handle].m_header->m_entries;
		offset = entries[entry].m_baseOffset + *(uint32_t*) (offsets + index * 8);
		size = *(uint32_t*) (offsets + index * 8 + 4) - entries[entry].m_baseOffset;
		if (MechSeek(g_archiveSlots[p_handle].m_fd, offset, SEEK_SET) == -1) {
			return -1;
		}

		if (ArchiveRead(g_archiveSlots[p_handle].m_fd, (uint8_t*) p_data, size) != size) {
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
int32_t GetArchiveItemOffset(int32_t p_handle, char* p_name, uint16_t p_index, int32_t* p_fd)
{
	int16_t entry;
	uint16_t index;
	uint8_t* offsets;
	ArchiveEntry* entries;
	int32_t offset;

	entry = FindArchiveEntry(p_handle, p_name);
	if (entry != -1) {
		if (g_archiveSlots[p_handle].m_entries[entry].m_data == NULL && LoadArchiveEntries(p_handle) == -1) {
			return 0;
		}

		index = p_index;
		offsets = (uint8_t*) g_archiveSlots[p_handle].m_entries[entry].m_data + 0x16;
		entries = g_archiveSlots[p_handle].m_header->m_entries;
		offset = entries[entry].m_baseOffset + *(uint32_t*) (offsets + index * 8);
		*p_fd = g_archiveSlots[p_handle].m_fd;
		return offset;
	}
	else {
		return 0;
	}
}
