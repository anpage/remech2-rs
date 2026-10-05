#ifndef TMPACKDATABASE_H
#define TMPACKDATABASE_H

#include "collection.h"
#include "decomp.h"
#include "types.h"

#include <stdio.h>

enum {
	c_lzWindowSize = 0x1000
};

// SIZE 0x8c
class TMPackDataBase {
public:
	// SIZE 0x08
	class TMPackDBEntry {
	public:
		TMPackDBEntry(undefined4 p_offset, undefined4 p_size);
		~TMPackDBEntry();

		undefined4 m_offset; // 0x00
		undefined4 m_size;   // 0x04
	};

	TMPackDataBase(char* p_name);
	~TMPackDataBase();

	void DumpEntries();
	MechS32 GetEntryCount();
	TMPackDBEntry* GetEntry(MechS32 p_id);
	MechS32 GetDBItem(MechS32 p_id, void** p_data, MechS32* p_size);
	MechS32 GetDBItemLZ(MechS32 p_id, void** p_data, MechS32* p_size);
	MechS32 ReadDBItemData(MechS32 p_id, MechS32 p_offset, void* p_buffer, size_t p_size);
	MechS32 ReadDBItemLine(MechS32 p_id, MechS32 p_offset, MechChar* p_buffer);
	MechS32 ReadDBItemString(MechS32 p_id, MechS32 p_offset, MechChar* p_buffer);

private:
	MechChar m_name[0x80]; // 0x00
	FILE* m_file;          // 0x80
	MechS32 m_numEntries;  // 0x84
	Collection* m_entries; // 0x88
};

// The globals of tmpackdatabase.cpp that other units use.
extern MechU8 g_lzWindow[c_lzWindowSize];

#endif // TMPACKDATABASE_H
