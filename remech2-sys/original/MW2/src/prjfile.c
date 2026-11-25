#include "prjfile.h"

#include "files.h"
#include "types.h"

#include <string.h>
#include <strings.h>

// PROJ resource files: a header listing resource types, each with an index of resources by ID.
// One file can be open at a time. Memory comes from the allocator SetPrjAllocator installs.

PrjAllocFn g_prjAlloc = NULL;

PrjFreeFn g_prjFree = NULL;

PrjFile g_prjFiles[1];

void SetPrjAllocator(PrjAllocFn p_alloc, PrjFreeFn p_free)
{
	g_prjAlloc = p_alloc;
	g_prjFree = p_free;
}

void* PrjAlloc(uint32_t p_size)
{
	return g_prjAlloc(p_size);
}

void PrjFreeBlock(void* p_block)
{
	g_prjFree(p_block);
}

// Writes in chunks of at most 16K. Returns p_length, or -1 on a short write.
int32_t WritePrjBytes(int32_t p_fd, void* p_buffer, uint32_t p_length)
{
	uint32_t chunk;
	uint32_t left;
	uint8_t* buffer;

	left = p_length;
	buffer = p_buffer;
	while (left > 0) {
		chunk = left < 0x4000 ? left : 0x4000;

		if (MechWrite(p_fd, buffer, chunk) != chunk) {
			return -1;
		}

		left -= chunk;
		buffer += chunk;
	}

	return p_length;
}

// Reads in chunks of at most 16K. Returns p_length, or -1 on a short read.
int32_t ReadPrjBytes(int32_t p_fd, void* p_buffer, uint32_t p_length)
{
	uint32_t chunk;
	uint32_t left;
	uint8_t* buffer;

	left = p_length;
	buffer = p_buffer;
	while (left > 0) {
		chunk = left < 0x4000 ? left : 0x4000;

		if (MechRead(p_fd, buffer, chunk) != chunk) {
			return -1;
		}

		left -= chunk;
		buffer += chunk;
	}

	return p_length;
}

// Opens a PROJ file (p_mode 0: read-only, 2: read-write, which rejects a file marked 0xfe).
// Returns its slot, -1 on failure or -2 for a rejected file.
int32_t OpenPrjFile(const char* p_name, char p_mode)
{
	char tag[12];
	int32_t fd;
	uint32_t size;
	uint16_t slot;
	PrjHeader* header;

	for (slot = 0; slot < 1 && g_prjFiles[slot].m_open; slot++) {
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
			MechRead(fd, tag, 12);
			if (strncasecmp(tag, "PROJ", 4)) {
				return -1;
			}

			if ((uint8_t) tag[8] == 0xfe) {
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
			MechRead(fd, tag, 12);
			if (strncasecmp(tag, "PROJ", 4)) {
				return -1;
			}
		}
	}
	else {
		return -1;
	}

	MechSeek(fd, 0x10, 0);
	if (ReadPrjBytes(fd, &size, 4) != 4) {
		return -1;
	}

	size += 8;
	header = PrjAlloc(size);
	if (!header) {
		return -1;
	}

	MechSeek(fd, 0xc, 0);
	if (ReadPrjBytes(fd, header, size) != size) {
		PrjFreeBlock(header);
		return -1;
	}

	g_prjFiles[slot].m_fd = fd;
	g_prjFiles[slot].m_header = header;
	g_prjFiles[slot].m_open = true;
	memset(g_prjFiles[slot].m_name, 0, 0x20);
	strncpy(g_prjFiles[slot].m_name, p_name, 0x20);
	return slot;
}

int32_t ClosePrjFile(int32_t p_file)
{
	uint16_t i;

	PrjFreeBlock(g_prjFiles[p_file].m_header);
	g_prjFiles[p_file].m_header = NULL;
	for (i = 0; i < 32; i++) {
		if (g_prjFiles[p_file].m_types[i].m_index) {
			PrjFreeBlock(g_prjFiles[p_file].m_types[i].m_index);
			g_prjFiles[p_file].m_types[i].m_index = NULL;
		}
	}

	g_prjFiles[p_file].m_open = false;
	memset(g_prjFiles[p_file].m_name, 0, 0x20);
	return MechClose(g_prjFiles[p_file].m_fd);
}

// Returns the type's index in the header, or 0xffff.
uint16_t FindPrjType(int32_t p_file, const char* p_type)
{
	int8_t found;
	uint16_t count;
	PrjType* types;
	uint16_t i;

	found = false;
	count = g_prjFiles[p_file].m_header->m_typeCount;
	types = g_prjFiles[p_file].m_header->m_types;
	i = 0;
	while ((count > i) & !found) {
		if (strncmp(types[i].m_tag, p_type, 4) == 0) {
			found = true;
		}
		else {
			i++;
		}
	}

	if (found) {
		return i;
	}
	else {
		return 0xffff;
	}
}

// Loads every type's index.
int32_t LoadPrjIndexes(int32_t p_file)
{
	PrjType* types;
	uint16_t count;
	PrjIndex* index;
	uint16_t i;
	uint32_t size;

	count = g_prjFiles[p_file].m_header->m_typeCount;
	types = g_prjFiles[p_file].m_header->m_types;
	for (i = 0; i < count; i++) {
		if (types[i].m_indexOffset) {
			size = types[i].m_indexSize;
			index = PrjAlloc(size);
			if (!index) {
				return -1;
			}

			MechSeek(g_prjFiles[p_file].m_fd, types[i].m_indexOffset, 0);
			if (ReadPrjBytes(g_prjFiles[p_file].m_fd, index, size) != size) {
				PrjFreeBlock(index);
				return -1;
			}

			g_prjFiles[p_file].m_types[i].m_index = index;
		}
	}

	return 0;
}

int32_t GetPrjResourceSize(int32_t p_file, const char* p_type, uint16_t p_id)
{
	PrjIndexEntry* entries;
	int32_t size;
	PrjType* types;
	int16_t type;
	uint16_t id;

	if (!p_id) {
		return -1;
	}

	type = FindPrjType(p_file, p_type);
	if (type >= 0) {
		if (!g_prjFiles[p_file].m_types[type].m_index && LoadPrjIndexes(p_file) == -1) {
			return -1;
		}

		id = p_id;
		entries = g_prjFiles[p_file].m_types[type].m_index->m_entries;
		types = g_prjFiles[p_file].m_header->m_types;
		size = entries[id].m_end - types[type].m_indexBase;
		return size > 0 ? size : 0;
	}
	else {
		return -1;
	}
}

// Seeks the file to a resource, and returns its offset and size. The type test never fails (a
// 16-bit index compared with -1).
int32_t SeekPrjResource(int32_t p_file, const char* p_type, uint16_t p_id, int32_t* p_offset, int32_t* p_size)
{
	PrjIndexEntry* entries;
	PrjType* types;
	int32_t offset;
	uint16_t type;

	type = FindPrjType(p_file, p_type);
	// FindPrjType's -1 as the 16 bits it's kept in: compared with an int, as the original's
	// source has it, it never matches
	if (type == (uint16_t) -1) {
		return -1;
	}

	if (!g_prjFiles[p_file].m_types[type].m_index && LoadPrjIndexes(p_file) == -1) {
		return -1;
	}

	entries = g_prjFiles[p_file].m_types[type].m_index->m_entries;
	types = g_prjFiles[p_file].m_header->m_types;
	offset = types[type].m_indexBase + entries[p_id].m_offset;
	if (p_offset) {
		*p_offset = offset;
	}

	if (p_size) {
		*p_size = entries[p_id].m_end - types[type].m_indexBase;
	}

	MechSeek(g_prjFiles[p_file].m_fd, offset, 0);
	return 0;
}

int32_t ReadPrjAt(void* p_buffer, int32_t p_file, int32_t p_offset, uint32_t p_length)
{
	p_file = g_prjFiles[p_file].m_fd;
	MechSeek(p_file, p_offset, 0);
	return ReadPrjBytes(p_file, p_buffer, p_length);
}

int32_t ReadPrjResource(int32_t p_file, const char* p_type, uint16_t p_id, void* p_buffer)
{
	PrjIndexEntry* entries;
	PrjType* types;
	int32_t offset;
	int16_t type;
	uint32_t size;
	uint16_t id;

	type = FindPrjType(p_file, p_type);
	if (type != -1) {
		if (!g_prjFiles[p_file].m_types[type].m_index && LoadPrjIndexes(p_file) == -1) {
			return -1;
		}

		id = p_id;
		entries = g_prjFiles[p_file].m_types[type].m_index->m_entries;
		types = g_prjFiles[p_file].m_header->m_types;
		offset = types[type].m_indexBase + entries[id].m_offset;
		size = entries[id].m_end - types[type].m_indexBase;
		if (MechSeek(g_prjFiles[p_file].m_fd, offset, 0) == -1) {
			return -1;
		}

		if (ReadPrjBytes(g_prjFiles[p_file].m_fd, p_buffer, size) != size) {
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

// Returns a resource's offset in the file, and the file's descriptor.
int32_t GetPrjResourceOffset(int32_t p_file, const char* p_type, uint16_t p_id, int32_t* p_fd)
{
	PrjIndexEntry* entries;
	PrjType* types;
	int32_t offset;
	int16_t type;
	uint16_t id;

	type = FindPrjType(p_file, p_type);
	if (type != -1) {
		if (!g_prjFiles[p_file].m_types[type].m_index && LoadPrjIndexes(p_file) == -1) {
			return 0;
		}

		id = p_id;
		entries = g_prjFiles[p_file].m_types[type].m_index->m_entries;
		types = g_prjFiles[p_file].m_header->m_types;
		offset = types[type].m_indexBase + entries[id].m_offset;
		*p_fd = g_prjFiles[p_file].m_fd;
		return offset;
	}
	else {
		return 0;
	}
}
