#include "prjfile.h"

#include "decomp.h"
#include "files.h"
#include "types.h"

#include <string.h>
#include <strings.h>

// PROJ resource files: a header listing resource types, each with an index of resources by ID.
// One file can be open at a time. Memory comes from the allocator SetPrjAllocator installs.

DECOMP_SIZE_ASSERT(PrjType, 0x18)
DECOMP_SIZE_ASSERT(PrjIndexEntry, 0x08)
DECOMP_SIZE_ASSERT(PrjTypeSlot, 0x08)
DECOMP_SIZE_ASSERT(PrjFile, 0x14a)

// GLOBAL: MW2 0x100ae734
PrjAllocFn g_prjAlloc = NULL;

// GLOBAL: MW2 0x100ae738
PrjFreeFn g_prjFree = NULL;

// GLOBAL: MW2 0x100c3120
PrjFile g_prjFiles[1];

// FUNCTION: MW2 0x10071ad0
void SetPrjAllocator(PrjAllocFn p_alloc, PrjFreeFn p_free)
{
	g_prjAlloc = p_alloc;
	g_prjFree = p_free;
}

// FUNCTION: MW2 0x10071aeb
void* PrjAlloc(MechU32 p_size)
{
	return g_prjAlloc(p_size);
}

// FUNCTION: MW2 0x10071b08
void PrjFreeBlock(void* p_block)
{
	g_prjFree(p_block);
}

// Writes in chunks of at most 16K. Returns p_length, or -1 on a short write.
// Stack-slot permutation: chunk, left and buffer.
// FUNCTION: MW2 0x10071b20
MechS32 WritePrjBytes(MechS32 p_fd, void* p_buffer, MechU32 p_length)
{
	MechU32 chunk;
	MechU32 left;
	MechU8* buffer;

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
// Stack-slot permutation: chunk, left and buffer.
// FUNCTION: MW2 0x10071b9e
MechS32 ReadPrjBytes(MechS32 p_fd, void* p_buffer, MechU32 p_length)
{
	MechU32 chunk;
	MechU32 left;
	MechU8* buffer;

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
// Stack-slot permutation: tag, fd, size, slot and header.
// FUNCTION: MW2 0x10071c1c
MechS32 OpenPrjFile(const MechChar* p_name, MechChar p_mode)
{
	MechChar tag[12];
	MechS32 fd;
	MechU32 size;
	MechU16 slot;
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

			if ((MechU8) tag[8] == 0xfe) {
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
	g_prjFiles[slot].m_open = TRUE;
	memset(g_prjFiles[slot].m_name, 0, 0x20);
	strncpy(g_prjFiles[slot].m_name, p_name, 0x20);
	return slot;
}

// FUNCTION: MW2 0x10071f09
MechS32 ClosePrjFile(MechS32 p_file)
{
	MechU16 i;

	PrjFreeBlock(g_prjFiles[p_file].m_header);
	g_prjFiles[p_file].m_header = NULL;
	for (i = 0; i < 32; i++) {
		if (g_prjFiles[p_file].m_types[i].m_index) {
			PrjFreeBlock(g_prjFiles[p_file].m_types[i].m_index);
			g_prjFiles[p_file].m_types[i].m_index = NULL;
		}

		if (g_prjFiles[p_file].m_types[i].m_unk0x04) {
			PrjFreeBlock(g_prjFiles[p_file].m_types[i].m_unk0x04);
			g_prjFiles[p_file].m_types[i].m_unk0x04 = NULL;
		}
	}

	g_prjFiles[p_file].m_open = FALSE;
	memset(g_prjFiles[p_file].m_name, 0, 0x20);
	return MechClose(g_prjFiles[p_file].m_fd);
}

// Returns the type's index in the header, or 0xffff.
// Operand order: the original tests count > i with setg.
// FUNCTION: MW2 0x100720b7
MechU16 FindPrjType(MechS32 p_file, const MechChar* p_type)
{
	MechS8 found;
	MechU16 count;
	PrjType* types;
	MechU16 i;

	found = FALSE;
	count = g_prjFiles[p_file].m_header->m_typeCount;
	types = g_prjFiles[p_file].m_header->m_types;
	i = 0;
	while ((count > i) & !found) {
		if (strncmp(types[i].m_tag, p_type, 4) == 0) {
			found = TRUE;
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
// Stack-slot permutation: types, count, index, i and size.
// FUNCTION: MW2 0x1007218f
MechS32 LoadPrjIndexes(MechS32 p_file)
{
	PrjType* types;
	MechU16 count;
	PrjIndex* index;
	MechU16 i;
	MechU32 size;

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

// Stack-slot permutation: entries, size, types, type and id.
// FUNCTION: MW2 0x100722f5
MechS32 GetPrjResourceSize(MechS32 p_file, const MechChar* p_type, MechU16 p_id)
{
	PrjIndexEntry* entries;
	MechS32 size;
	PrjType* types;
	MechS16 type;
	MechU16 id;

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
// Stack-slot permutation: entries, types, offset and type.
// FUNCTION: MW2 0x10072405
MechS32 SeekPrjResource(MechS32 p_file, const MechChar* p_type, MechU16 p_id, MechS32* p_offset, MechS32* p_size)
{
	PrjIndexEntry* entries;
	PrjType* types;
	MechS32 offset;
	MechU16 type;

	type = FindPrjType(p_file, p_type);
	// FindPrjType's -1 as the 16 bits it's kept in: compared with an int, as the original's
	// source has it, it never matches
	if (type == (MechU16) -1) {
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

// FUNCTION: MW2 0x10072560
MechS32 ReadPrjAt(void* p_buffer, MechS32 p_file, MechS32 p_offset, MechU32 p_length)
{
	p_file = g_prjFiles[p_file].m_fd;
	MechSeek(p_file, p_offset, 0);
	return ReadPrjBytes(p_file, p_buffer, p_length);
}

// Stack-slot permutation: entries, types, offset, type, size and id.
// FUNCTION: MW2 0x100725ae
MechS32 ReadPrjResource(MechS32 p_file, const MechChar* p_type, MechU16 p_id, void* p_buffer)
{
	PrjIndexEntry* entries;
	PrjType* types;
	MechS32 offset;
	MechS16 type;
	MechU32 size;
	MechU16 id;

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
// Stack-slot permutation: entries, types, offset, type and id.
// FUNCTION: MW2 0x10072734
MechS32 GetPrjResourceOffset(MechS32 p_file, const MechChar* p_type, MechU16 p_id, MechS32* p_fd)
{
	PrjIndexEntry* entries;
	PrjType* types;
	MechS32 offset;
	MechS16 type;
	MechU16 id;

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
