#include "tmpackdatabase.h"

#include "files.h"
#include "log.h"
#include "windowstate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The ring buffer of GetDBItemLZ's LZSS decoder.
uint8_t g_lzWindow[c_lzWindowSize];

// Prints the database's entries to stdout. Unused.
void TMPackDataBase::DumpEntries()
{
	TMPackDBEntry* entry;
	int32_t i;

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

TMPackDataBase::TMPackDataBase(char* p_name)
{
	TMPackDBEntry* entry;
	TMPackDBEntry* next;
	TMPackDBEntry* current;
	uint32_t offset;

	strcpy(m_name, p_name);
	m_data = (uint8_t*) MechReadFile(g_primaryHeap, m_name);
	if (m_data == NULL) {
		MechLogErrorf("Could not open mpack DB: %s\n", m_name);
		exit(1);
	}

	m_size = MechHeapSize(g_primaryHeap, m_data);
	if (m_size < 4) {
		MechLogErrorf("mpack DB file too small: %s", m_name);
		exit(1);
	}

	memcpy(&m_numEntries, m_data, 4);
	if (m_numEntries < 1 || m_numEntries > (m_size - 4) / 4) {
		MechLogErrorf("mpack DB has a bad entry count: %s", m_name);
		exit(1);
	}

	CreateCollection(&m_entries, m_numEntries, NULL, 4, NULL);
	for (int32_t i = 0; i < m_numEntries; i++) {
		memcpy(&offset, m_data + 4 + i * 4, 4);
		entry = new TMPackDBEntry(offset, 0);
		ExpandCollection(m_entries, entry);
	}

	current = (TMPackDBEntry*) CollectionGet(m_entries, 0);
	next = current;
	for (int32_t i = 1; i < m_numEntries; i++) {
		next = (TMPackDBEntry*) CollectionGet(m_entries, i);
		current->m_size = next->m_offset - current->m_offset;
		if (i < m_numEntries - 1) {
			current = next;
		}
	}

	next->m_size = m_size - next->m_offset;
}

TMPackDataBase::~TMPackDataBase()
{
	TMPackDBEntry* entry;
	int32_t i;

	for (i = 0; i < m_entries->m_count; i++) {
		entry = (TMPackDBEntry*) CollectionGet(m_entries, i);
		delete entry;
	}

	MechHeapFree(g_primaryHeap, m_entries->m_items);
	MechHeapFree(g_primaryHeap, m_entries);
	MechHeapFree(g_primaryHeap, m_data);
}

int32_t TMPackDataBase::GetEntryCount()
{
	return m_entries->m_count;
}

// The ids are 1-based.
TMPackDataBase::TMPackDBEntry* TMPackDataBase::GetEntry(int32_t p_id)
{
	TMPackDBEntry* entry;

	if (p_id > m_entries->m_count || p_id < 0) {
		return NULL;
	}

	entry = (TMPackDBEntry*) CollectionGet(m_entries, p_id - 1);
	return entry;
}

int32_t TMPackDataBase::GetDBItem(int32_t p_id, void** p_data, int32_t* p_size)
{
	TMPackDBEntry* entry;
	void* data;

	entry = GetEntry(p_id);
	if (entry == NULL || entry->m_offset > (uint32_t) m_size || entry->m_size > (uint32_t) m_size - entry->m_offset) {
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

	memcpy(data, m_data + entry->m_offset, entry->m_size);
	return 0;
}

int32_t TMPackDataBase::GetDBItemLZ(int32_t p_id, void** p_data, int32_t* p_size)
{
	uint32_t flags;
	int32_t remaining;
	TMPackDBEntry* entry;
	uint8_t* dst;
	uint8_t* src;
	uint8_t* out;
	int32_t n;
	int32_t pos;

	entry = GetEntry(p_id);
	if (entry == NULL) {
		return 1;
	}

	pos = entry->m_offset;
	if (pos < 0 || pos > m_size - 4) {
		return 1;
	}

	memcpy(&remaining, m_data + pos, 4);
	pos += 4;

	out = (uint8_t*) MechHeapAlloc(g_primaryHeap, remaining);
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
			flags = ByteAt(pos++);
			flags |= 0x100;
		}

		if (flags & 1) {
			remaining--;
			*dst = ByteAt(pos++);
			*out++ = *dst++;
			if (dst == g_lzWindow + c_lzWindowSize) {
				dst = g_lzWindow;
			}
		}
		else {
			n = ByteAt(pos++);
			n += ByteAt(pos++) << 8;
			src = g_lzWindow + (n & (c_lzWindowSize - 1));
			n = (((uint32_t) n >> 12) & 0xf) + 3;
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
int32_t TMPackDataBase::ReadDBItemData(int32_t p_id, int32_t p_offset, void* p_buffer, size_t p_size)
{
	TMPackDBEntry* entry = GetEntry(p_id);
	if (entry == NULL) {
		return 1;
	}

	int32_t pos = entry->m_offset + p_offset;

	if (pos < 0 || p_size > (size_t) m_size || pos > m_size - (int32_t) p_size) {
		return 1;
	}

	memcpy(p_buffer, m_data + pos, p_size);
	return 0;
}

// Reads a line; returns 2 if it ended at a NUL rather than a newline.
int32_t TMPackDataBase::ReadDBItemLine(int32_t p_id, int32_t p_offset, char* p_buffer)
{
	TMPackDBEntry* entry;
	char c;
	int32_t i;
	int32_t pos;

	entry = GetEntry(p_id);
	if (entry == NULL) {
		return 1;
	}

	pos = entry->m_offset + p_offset;
	i = 0;
	while ((c = ByteAt(pos++)) != '\n' && c != '\0') {
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
int32_t TMPackDataBase::ReadDBItemString(int32_t p_id, int32_t p_offset, char* p_buffer)
{
	TMPackDBEntry* entry;
	char c;
	int32_t i;
	int32_t pos;

	entry = GetEntry(p_id);
	if (entry == NULL) {
		return 1;
	}

	pos = entry->m_offset + p_offset;
	i = 0;
	while ((c = ByteAt(pos++)) != '\0') {
		p_buffer[i] = c;
		i++;
	}

	p_buffer[i] = '\0';
	return 0;
}

int32_t TMPackDataBase::ByteAt(int32_t p_offset)
{
	return p_offset >= 0 && p_offset < m_size ? m_data[p_offset] : -1;
}

TMPackDataBase::TMPackDBEntry::TMPackDBEntry(uint32_t p_offset, uint32_t p_size)
{
	m_offset = p_offset;
	m_size = p_size;
}

TMPackDataBase::TMPackDBEntry::~TMPackDBEntry()
{
}
