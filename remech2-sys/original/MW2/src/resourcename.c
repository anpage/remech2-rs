/* Resources by name: the TABL tables of resource names, and loading a resource by id or name, or
   else from a file. */
#include "resourcename.h"

#include "config.h"
#include "files.h"
#include "loadres.h"
#include "mw2prj.h"
#include "prjfile.h"
#include "resourceref.h"
#include "simmain.h"
#include "staticmem.h"
#include "types.h"

#include <string.h>
#include <strings.h>

// Returns the id of the resource named p_name in TABL resource p_table, or -1. The table's entries
// are 12 bytes from 0x0c: a name of 10 bytes, each stored as 0x100 minus the character, and the id.
int32_t FindResourceIdByName(int32_t p_table, char* p_name)
{
	int32_t j;
	uint8_t* header;
	int32_t result;
	uint8_t* table;
	uint8_t* entry;
	int32_t i;
	int32_t c;
	char name[12];

	result = -1;
	table = LoadCachedResource(g_mw2PrjHandle, p_table, g_resourceTypeTags[c_resTagTable]);
	if (table) {
		header = table;
		entry = table + 0xc;
		for (j = 0; j < 10; j++) {
			c = (int8_t) entry[j];
			c = 0x100 - c;
			name[j] = c;
		}

		i = 0;
		while (i < *(int16_t*) (header + 8) && strcasecmp(name, p_name)) {
			entry += 0xc;
			i++;
			for (j = 0; j < 10; j++) {
				c = (int8_t) entry[j];
				c = 0x100 - c;
				name[j] = c;
			}
		}

		if (!strcasecmp(name, p_name)) {
			result = *(int16_t*) (entry + 0xa);
		}

		UnlockCachedResource(p_table, g_resourceTypeTags[c_resTagTable]);
	}

	return result;
}

// Loads a resource of type p_type by p_ref's id, or by its name when the id is -1 (looked up in TABL
// resource p_table), or else the file of that name with extension p_ext. With p_poolTag, the data
// is copied into the static pool. Stores the id found (-1 for a file) and returns the data.
void* LoadResourceByRef(
	ResourceRef* p_ref,
	const char* p_type,
	const char* p_ext,
	int32_t p_table,
	int32_t* p_size,
	uint32_t* p_poolTag
)
{
	void* loaded;
	int32_t fromResource;
	int32_t handle;
	int32_t id;
	void* data;
	char name[16];

	data = NULL;
	loaded = NULL;
	fromResource = false;
	id = p_ref->m_id;
	if (id == -1) {
		strncpy(name, p_ref->m_name, 12);
		name[12] = '\0';
		id = FindResourceIdByName(p_table, name);
	}

	if (id != -1) {
		data = LoadCachedResource(g_mw2PrjHandle, id, p_type);
		*p_size = GetPrjResourceSize(g_mw2PrjHandle, p_type, id);
		if (data) {
			fromResource = true;
		}
	}

	if (!data) {
		id = -1;
		name[8] = '\0';
		strncat(name, p_ext, 4);
		handle = LoadFile(BuildGamePath(name), p_size, &data, NULL);
		if (handle == -1) {
			data = NULL;
		}
		else {
			MechClose(handle);
			fromResource = false;
		}
	}

	if (data && p_poolTag && *p_size > 0) {
		loaded = data;
		data = StaticPoolAlloc(*p_size, *p_poolTag);
		if (data) {
			memcpy(data, loaded, *p_size);
		}

		if (fromResource) {
			UnlockCachedResource(id, p_type);
		}
		else {
			MechHeapFree(g_primaryHeap, loaded);
		}
	}

	p_ref->m_id = id;
	return data;
}
