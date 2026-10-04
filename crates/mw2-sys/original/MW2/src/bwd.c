#include "bwd.h"

#include "bwdkeywords.h"
#include "bwdstreamkey.h"
#include "config.h"
#include "decomp.h"
#include "error.h"
#include "files.h"
#include "loadres.h"
#include "mw2prj.h"
#include "overlay.h"
#include "resourcename.h"
#include "simmain.h"
#include "types.h"

#include <io.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

DECOMP_SIZE_ASSERT(BwdNode, 0x08)
DECOMP_SIZE_ASSERT(BwdHeader, 0x0c)
DECOMP_SIZE_ASSERT(BwdStream, 0x1f)

// Set by a command-line switch: OpenBwdStream reads every stream from a file, even one with a
// resource id.
// GLOBAL: MW2 0x100a5bec
MechS32 g_streamsFromFiles = 0;

// Set by a command-line switch: OpenBwdStream logs each stream it opens to mw2debug.txt.
// GLOBAL: MW2 0x100a5bf0
MechS32 g_logStreams = 0;

// Only LogKeywordName's dead first store takes its address.
// GLOBAL: MW2 0x10109c40
MechChar g_unk0x10109c40[1];

// Opens the BWD stream p_key names into p_stream: from a file when the key has no resource id
// (a name without an extension gets ".BWD"), or always with g_streamsFromFiles, and otherwise, or
// when there is no such file, from the resource file. Returns p_stream, or NULL.
// Stack-slot permutation: line, result, id, dot, file, size and data.
// FUNCTION: MW2 0x1003fb00
BwdStream* OpenBwdStream(BwdStreamKey* p_key, BwdStream* p_stream)
{
	MechChar line[256];
	MechS32 size;
	BwdStream* result;
	MechS32 file;
	MechChar* dot;
	BwdHeader* header;
	MechS32 id;
	void* data;

	result = NULL;
	data = NULL;
	id = p_key->m_id;
	strncpy(p_stream->m_name, p_key->m_name, 8);
	p_stream->m_name[8] = '\0';
	if (g_logStreams) {
		if (p_key->m_name[0]) {
			sprintf(line, "\n%s: ", p_key->m_name);
		}
		else {
			sprintf(line, "\nPrj file ID# %d: ", p_key->m_id);
		}

		LogDebugLine(line);
	}

	if (id != -2 && id == -1) {
		id = FindResourceIdByName(0xe, p_key->m_name);
		if (id == -1) {
			Error(0x3d, "%s ID %d", p_key->m_name, p_key->m_id, 0);
		}
	}

	if (id == -2 || id == -1 || g_streamsFromFiles) {
		p_stream->m_fromResource = 0;
		p_stream->m_id = -1;
		dot = strchr(p_key->m_name, '.');
		if (!dot) {
			strcat(p_key->m_name, ".");
			strcat(p_key->m_name, g_bwdExtension);
		}

		file = LoadFile(BuildGamePath(p_key->m_name), &size, &data, NULL);
		if (file != -1) {
			_close(file);
		}
		else {
			if (!g_streamsFromFiles) {
				Error(0x29, "%s ID %d", p_key->m_name, p_key->m_id, 0);
			}

			data = NULL;
		}
	}

	if (!data && id != -1) {
		p_stream->m_fromResource = 1;
		p_stream->m_id = id;
		data = LoadCachedResource(g_mw2PrjHandle, id, g_resourceTypeTags[c_resTagBwd], 1);
		if (!data) {
			Error(0x2a, "%s ID %d", p_key->m_name, p_key->m_id, 0);
		}
	}

	if (data) {
		header = data;
		if (header->m_type == g_bwdTypeCodes[0]) {
			p_stream->m_size = header->m_size;
			p_stream->m_maxNodeSize = header->m_maxNodeSize;
			p_stream->m_data = data;
			p_stream->m_current = data;
			result = p_stream;
		}
		else {
			Error(0x2b, "%s ID %d", p_key->m_name, p_key->m_id, 0);
		}
	}

	return result;
}

// Steps p_stream to its next node and returns it, or NULL at the end of the stream or on a node
// of a bad size.
// Stack-slot permutation: next and result.
// FUNCTION: MW2 0x1003fdfb
BwdNode* GetNextNode(BwdStream* p_stream)
{
	BwdNode* next;
	MechS32 size;
	BwdNode* result;

	result = NULL;
	if (p_stream->m_current->m_type == g_bwdTypeCodes[0]) {
		size = sizeof(BwdHeader);
	}
	else {
		size = p_stream->m_current->m_size;
	}

	next = (BwdNode*) ((MechU8*) p_stream->m_current + size);
	if ((MechU8*) next - (MechU8*) p_stream->m_data < p_stream->m_size) {
		if (next->m_size <= p_stream->m_maxNodeSize && next->m_size >= sizeof(BwdNode)) {
			p_stream->m_current = next;
			result = next;
		}
		else {
			Error(0x2c, "%s ID %d", p_stream->m_name, p_stream->m_id, 0);
		}
	}

	return result;
}

// Frees a stream's data, or releases its resource.
// FUNCTION: MW2 0x1003feb8
void UnloadResource(BwdStream* p_stream)
{
	if (p_stream) {
		if (p_stream->m_fromResource == 0) {
			MechHeapFree(g_primaryHeap, p_stream->m_data);
		}
		else {
			FreeCachedResource(p_stream->m_id, g_resourceTypeTags[c_resTagBwd]);
		}
	}
}

// Returns the name of the keyword with tag p_code, or "unknown".
// FUNCTION: MW2 0x1003ff09
MechChar* GetKeywordName(MechU32 p_code)
{
	MechS32 i;

	for (i = 0; i < 0x48; i++) {
		if (g_bwdTypeCodes[i] == p_code) {
			break;
		}
	}

	if (i < 0x48) {
		return g_bwdKeywordNames[i];
	}
	else {
		return "unknown";
	}
}

// Appends a line to mw2debug.txt and shows it on screen.
// FUNCTION: MW2 0x1003ff75
void LogDebugLine(MechChar* p_text)
{
	FILE* file;

	file = MechFopen("mw2debug.txt", "a");
	if (file) {
		fprintf(file, "%s", p_text);
	}
	fclose(file);
	MonoPrint(p_text);
}

// Logs a keyword's name with LogDebugLine.
// Stack-slot permutation: line and name.
// FUNCTION: MW2 0x1003ffcf
void LogKeywordName(MechU32 p_code)
{
	MechChar line[256];
	MechChar* name;

	name = g_unk0x10109c40;
	name = GetKeywordName(p_code);
	sprintf(line, "%s, ", name);
	LogDebugLine(line);
}
