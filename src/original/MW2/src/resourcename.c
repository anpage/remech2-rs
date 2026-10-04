/* Resources by name: the TABL tables of resource names, and loading a resource by id or name, or
   else from a file. */
#include "resourcename.h"

#include "config.h"
#include "loadres.h"
#include "mw2prj.h"
#include "prjfile.h"
#include "resourceref.h"
#include "simmain.h"
#include "staticmem.h"
#include "types.h"

#include <io.h>
#include <string.h>
#include <windows.h>

// Returns the id of the resource named p_name in TABL resource p_table, or -1. The table's entries
// are 12 bytes from 0x0c: a name of 10 bytes, each stored as 0x100 minus the character, and the id.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x100737e0
MechS32 FindResourceIdByName(MechS32 p_table, MechChar* p_name)
{
	MechS32 j;
	MechU8* header;
	MechS32 result;
	MechU8* table;
	MechU8* entry;
	MechS32 i;
	MechS32 c;
	MechChar name[12];

	result = -1;
	table = LoadCachedResource(g_mw2PrjHandle, p_table, g_resourceTypeTags[c_resTagTable], 1);
	if (table) {
		header = table;
		entry = table + 0xc;
		for (j = 0; j < 10; j++) {
			c = (MechS8) entry[j];
			c = 0x100 - c;
			name[j] = c;
		}

		i = 0;
		while (i < *(MechS16*) (header + 8) && _strcmpi(name, p_name)) {
			entry += 0xc;
			i++;
			for (j = 0; j < 10; j++) {
				c = (MechS8) entry[j];
				c = 0x100 - c;
				name[j] = c;
			}
		}

		if (!_strcmpi(name, p_name)) {
			result = *(MechS16*) (entry + 0xa);
		}

		UnlockCachedResource(p_table, g_resourceTypeTags[c_resTagTable]);
	}

	return result;
}

// Loads a resource of type p_type by p_ref's id, or by its name when the id is -1 (looked up in TABL
// resource p_table), or else the file of that name with extension p_ext. With p_poolTag, the data
// is copied into the static pool. Stores the id found (-1 for a file) and returns the data.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x10073922
void* LoadResourceByRef(
	ResourceRef* p_ref,
	const char* p_type,
	const char* p_ext,
	MechS32 p_table,
	MechS32* p_size,
	MechU32* p_poolTag
)
{
	void* loaded;
	MechS32 fromResource;
	MechS32 handle;
	MechS32 id;
	void* data;
	MechChar name[16];

	data = NULL;
	loaded = NULL;
	fromResource = FALSE;
	id = p_ref->m_id;
	if (id == -1) {
		strncpy(name, p_ref->m_name, 12);
		name[12] = '\0';
		id = FindResourceIdByName(p_table, name);
	}

	if (id != -1) {
		data = LoadCachedResource(g_mw2PrjHandle, id, p_type, 0);
		*p_size = GetPrjResourceSize(g_mw2PrjHandle, p_type, id);
		if (data) {
			fromResource = TRUE;
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
			_close(handle);
			fromResource = FALSE;
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
			HeapFree(g_primaryHeap, HEAP_NO_SERIALIZE, loaded);
		}
	}

	p_ref->m_id = id;
	return data;
}
