#ifndef TMPACKDATABASE_H
#define TMPACKDATABASE_H

#include "collection.h"
#include "types.h"

#include <stdio.h>

enum {
	c_lzWindowSize = 0x1000
};

class TMPackDataBase {
public:
	class TMPackDBEntry {
	public:
		TMPackDBEntry(uint32_t p_offset, uint32_t p_size);
		~TMPackDBEntry();

		uint32_t m_offset;
		uint32_t m_size;
	};

	TMPackDataBase(char* p_name);
	~TMPackDataBase();

	void DumpEntries();
	int32_t GetEntryCount();
	TMPackDBEntry* GetEntry(int32_t p_id);
	int32_t GetDBItem(int32_t p_id, void** p_data, int32_t* p_size);
	int32_t GetDBItemLZ(int32_t p_id, void** p_data, int32_t* p_size);
	int32_t ReadDBItemData(int32_t p_id, int32_t p_offset, void* p_buffer, size_t p_size);
	int32_t ReadDBItemLine(int32_t p_id, int32_t p_offset, char* p_buffer);
	int32_t ReadDBItemString(int32_t p_id, int32_t p_offset, char* p_buffer);

private:
	int32_t ByteAt(int32_t p_offset);

	char m_name[0x80];
	uint8_t* m_data;
	int32_t m_size;
	int32_t m_numEntries;
	Collection* m_entries;
};

// The globals of tmpackdatabase.cpp that other units use.
extern uint8_t g_lzWindow[c_lzWindowSize];

#endif // TMPACKDATABASE_H
