#include "resourcename.h"

#include "mw2prj.h"
#include "resourcecache.h"
#include "types.h"

#include <string.h>
#include <strings.h>

// Resource names: each resource type has a TABL resource listing its entries' names
// (stored negated) and ids.

typedef struct ResourceEntry {
	char m_name[10]; // 0x00
	int16_t m_id;    // 0x0a
} ResourceEntry;

typedef struct ResourceTable {
	uint8_t m_reserved0x00[0x08]; // 0x00
	int16_t m_count;              // 0x08
	uint16_t m_reserved0x0a;      // 0x0a
	ResourceEntry m_entries[1];   // 0x0c
} ResourceTable;

int32_t FindResourceIdByName(int32_t p_type, char* p_name)
{
	int32_t j;
	ResourceTable* table;
	int32_t id;
	ResourceTable* data;
	ResourceEntry* entry;
	int32_t i;
	int32_t c;
	char name[12];

	id = -1;
	data = LoadCachedResource(g_mw2PrjHandle, p_type, g_resourceTypeTags[c_resTagTable]);
	if (data != NULL) {
		table = data;
		entry = data->m_entries;
		for (j = 0; j < 10; j++) {
			c = entry->m_name[j];
			c = 0x100 - c;
			name[j] = c;
		}

		i = 0;
		while (i < table->m_count && strcasecmp(name, p_name)) {
			entry++;
			i++;
			for (j = 0; j < 10; j++) {
				c = entry->m_name[j];
				c = 0x100 - c;
				name[j] = c;
			}
		}

		if (!strcasecmp(name, p_name)) {
			id = entry->m_id;
		}

		UnlockCachedResource(p_type, g_resourceTypeTags[c_resTagTable]);
	}

	return id;
}

char* FindResourceNameById(int32_t p_type, int32_t p_id, char* p_buffer)
{
	int32_t j;
	ResourceTable* table;
	ResourceTable* data;
	ResourceEntry* entry;
	char* result;
	int32_t i;
	int32_t c;

	result = NULL;
	if (p_buffer != NULL) {
		data = LoadCachedResource(g_mw2PrjHandle, p_type, g_resourceTypeTags[c_resTagTable]);
		if (data != NULL) {
			table = data;
			entry = data->m_entries;
			i = 0;
			while (i < table->m_count && entry->m_id != p_id) {
				entry++;
				i++;
			}

			if (entry->m_id == p_id) {
				result = p_buffer;
				for (j = 0; j < 9; j++) {
					c = entry->m_name[j];
					c = 0x100 - c;
					result[j] = c;
				}
				result[8] = '\0';
			}

			UnlockCachedResource(p_type, g_resourceTypeTags[c_resTagTable]);
		}
	}

	return result;
}
