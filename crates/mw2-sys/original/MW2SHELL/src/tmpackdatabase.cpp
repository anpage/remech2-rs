#include "files.h"
#include "tmpackdatabase.h"

#include "windowstate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

DECOMP_SIZE_ASSERT(TMPackDataBase, 0x8c)
DECOMP_SIZE_ASSERT(TMPackDataBase::TMPackDBEntry, 0x08)

enum {
	c_lzWindowSize = 0x1000
};

// The ring buffer of GetDBItemLZ's LZSS decoder.
// GLOBAL: MW2SHELL 0x10094c90
MechU8 g_lzWindow[c_lzWindowSize];

// Prints the database's entries to stdout. Unused.
// FUNCTION: MW2SHELL 0x10047bc0
void TMPackDataBase::DumpEntries()
{
	TMPackDBEntry* entry;
	MechS32 i;

	fprintf(stdout, "MPack File: %s Items: %d\n", m_name, m_numEntries);
	for (i = 0; i < m_entries->m_count; i++) {
		entry = (TMPackDBEntry*) CollectionGet(m_entries, i);
		fprintf(
			stdout,
			"Offset:  %d (hex=%x), Size: %d (hex=%x)\n",
			entry->m_offset,
			entry->m_offset,
			entry->m_size,
			entry->m_size
		);
	}
}

// Stack-slot permutation: result, entry, fileSize, next, current, count, i and offset.
// FUNCTION: MW2SHELL 0x10047c6c
TMPackDataBase::TMPackDataBase(char* p_name)
{
	MechS32 result;
	TMPackDBEntry* entry;
	MechS32 fileSize;
	TMPackDBEntry* next;
	TMPackDBEntry* current;
	size_t count;
	MechS32 i;
	undefined4 offset;

	strcpy(m_name, p_name);
	m_file = MechFopen(m_name, "rb");
	if (m_file == NULL) {
		fprintf(stderr, "Could not open mpack DB: %s\n", m_name);
		fflush(stderr);
		exit(1);
	}

	setvbuf(m_file, NULL, _IOFBF, 0x4000);
	count = fread(&m_numEntries, 4, 1, m_file);
	result = CreateCollection(&m_entries, m_numEntries, NULL, 4, NULL);

	for (i = 0; i < m_numEntries; i++) {
		count = fread(&offset, 4, 1, m_file);
		entry = new TMPackDBEntry(offset, 0);
		result = ExpandCollection(m_entries, entry);
	}

	fseek(m_file, 0, SEEK_END);
	fileSize = ftell(m_file);

	current = (TMPackDBEntry*) CollectionGet(m_entries, 0);
	for (i = 1; i < m_numEntries; i++) {
		next = (TMPackDBEntry*) CollectionGet(m_entries, i);
		current->m_size = next->m_offset - current->m_offset;
		if (i < m_numEntries - 1) {
			current = next;
		}
	}

	next->m_size = fileSize - next->m_offset;
}

// Stack-slot permutation: entry and i.
// FUNCTION: MW2SHELL 0x10047f03
TMPackDataBase::~TMPackDataBase()
{
	TMPackDBEntry* entry;
	MechS32 i;

	for (i = 0; i < m_entries->m_count; i++) {
		entry = (TMPackDBEntry*) CollectionGet(m_entries, i);
		delete entry;
	}

	MechHeapFree(g_primaryHeap, m_entries->m_items);
	MechHeapFree(g_primaryHeap, m_entries);
	fclose(m_file);
}

// FUNCTION: MW2SHELL 0x10047fd4
MechS32 TMPackDataBase::GetEntryCount()
{
	return m_entries->m_count;
}

// The ids are 1-based.
// FUNCTION: MW2SHELL 0x10047ff6
TMPackDataBase::TMPackDBEntry* TMPackDataBase::GetEntry(MechS32 p_id)
{
	TMPackDBEntry* entry;

	if (p_id > m_entries->m_count || p_id < 0) {
		return NULL;
	}

	entry = (TMPackDBEntry*) CollectionGet(m_entries, p_id - 1);
	return entry;
}

// Stack-slot permutation: entry and data.
// FUNCTION: MW2SHELL 0x10048051
MechS32 TMPackDataBase::GetDBItem(MechS32 p_id, void** p_data, MechS32* p_size)
{
	TMPackDBEntry* entry;
	void* data;

	entry = GetEntry(p_id);
	if (entry == NULL) {
		*p_data = NULL;
		*p_size = 0;
		return 1;
	}

	data = MechHeapAlloc(g_primaryHeap, entry->m_size);
	if (data == NULL) {
		fprintf(stderr, "Out of memory in TMPackDataBaseObj :: GetDBItem\n");
		fflush(stderr);
		exit(1);
	}

	*p_data = data;
	*p_size = entry->m_size;
	fseek(m_file, entry->m_offset, SEEK_SET);
	fread(data, 1, entry->m_size, m_file);
	return 0;
}

// Stack-slot permutation: remaining, entry, dst and out.
// FUNCTION: MW2SHELL 0x1004813f
MechS32 TMPackDataBase::GetDBItemLZ(MechS32 p_id, void** p_data, MechS32* p_size)
{
	MechU32 flags;
	MechS32 remaining;
	TMPackDBEntry* entry;
	MechU8* dst;
	MechU8* src;
	MechU8* out;
	MechS32 n;

	entry = GetEntry(p_id);
	if (entry == NULL) {
		return 1;
	}

	n = fseek(m_file, entry->m_offset, SEEK_SET);
	if (n != 0) {
		return 1;
	}

	n = fread(&remaining, 4, 1, m_file);
	if (n != 1) {
		return 1;
	}

	out = (MechU8*) MechHeapAlloc(g_primaryHeap, remaining);
	if (out == NULL) {
		fprintf(stderr, "Out of memory in TMPackDataBaseObj :: GetDBItem\n");
		fflush(stderr);
		exit(1);
	}

	*p_data = out;
	*p_size = remaining;

	for (n = 0; n < c_lzWindowSize; n++) {
		g_lzWindow[n] = 0;
	}

	dst = g_lzWindow;
	flags = 2;
	while (remaining > 0) {
		flags >>= 1;
		if (flags == 1) {
			flags = fgetc(m_file);
			flags |= 0x100;
		}

		if (flags & 1) {
			remaining--;
			*dst = fgetc(m_file);
			*out++ = *dst++;
			if (dst == g_lzWindow + c_lzWindowSize) {
				dst = g_lzWindow;
			}
		}
		else {
			n = fgetc(m_file);
			n += fgetc(m_file) << 8;
			src = g_lzWindow + (n & (c_lzWindowSize - 1));
			n = (((MechU32) n >> 12) & 0xf) + 3;
			n &= c_lzWindowSize - 1;
			remaining -= n;
			while (n--) {
				*dst = *src++;
				*out++ = *dst++;
				if (dst == g_lzWindow + c_lzWindowSize) {
					dst = g_lzWindow;
				}
				if (src == g_lzWindow + c_lzWindowSize) {
					src = g_lzWindow;
				}
			}
		}
	}

	return 0;
}

// Reads p_size bytes of item p_id, from p_offset on.
// FUNCTION: MW2SHELL 0x100483c8
MechS32 TMPackDataBase::ReadDBItemData(MechS32 p_id, MechS32 p_offset, void* p_buffer, size_t p_size)
{
	TMPackDBEntry* entry;

	entry = GetEntry(p_id);
	if (entry == NULL) {
		return 1;
	}

	fseek(m_file, entry->m_offset + p_offset, SEEK_SET);
	fread(p_buffer, 1, p_size, m_file);
	return 0;
}

// Reads a line; returns 2 if it ended at a NUL rather than a newline.
// Stack-slot permutation: entry, c and i.
// FUNCTION: MW2SHELL 0x1004843e
MechS32 TMPackDataBase::ReadDBItemLine(MechS32 p_id, MechS32 p_offset, MechChar* p_buffer)
{
	TMPackDBEntry* entry;
	MechChar c;
	MechS32 i;

	entry = GetEntry(p_id);
	if (entry == NULL) {
		return 1;
	}

	fseek(m_file, entry->m_offset + p_offset, SEEK_SET);
	i = 0;
	while ((c = fgetc(m_file)) != '\n' && c != '\0') {
		p_buffer[i] = c;
		i++;
	}

	p_buffer[i] = '\0';
	if (c == '\0') {
		return 2;
	}

	return 0;
}

// Reads a NUL-terminated string.
// Stack-slot permutation: entry, c and i.
// FUNCTION: MW2SHELL 0x10048501
MechS32 TMPackDataBase::ReadDBItemString(MechS32 p_id, MechS32 p_offset, MechChar* p_buffer)
{
	TMPackDBEntry* entry;
	MechChar c;
	MechS32 i;

	entry = GetEntry(p_id);
	if (entry == NULL) {
		return 1;
	}

	fseek(m_file, entry->m_offset + p_offset, SEEK_SET);
	i = 0;
	while ((c = fgetc(m_file)) != '\0') {
		p_buffer[i] = c;
		i++;
	}

	p_buffer[i] = '\0';
	return 0;
}

// FUNCTION: MW2SHELL 0x100485a1
TMPackDataBase::TMPackDBEntry::TMPackDBEntry(undefined4 p_offset, undefined4 p_size)
{
	m_offset = p_offset;
	m_size = p_size;
}

// FUNCTION: MW2SHELL 0x100485cd
TMPackDataBase::TMPackDBEntry::~TMPackDBEntry()
{
}
