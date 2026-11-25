#include "bwd.h"

#include "bwdkeywords.h"
#include "bwdstreamkey.h"
#include "config.h"
#include "error.h"
#include "files.h"
#include "loadres.h"
#include "log.h"
#include "mw2prj.h"
#include "overlay.h"
#include "resourcename.h"
#include "simmain.h"
#include "types.h"

#include <stdio.h>
#include <string.h>

// Set by a command-line switch: OpenBwdStream reads every stream from a file, even one with a
// resource id.
int32_t g_streamsFromFiles = 0;

// Set by a command-line switch: OpenBwdStream logs each stream it opens
int32_t g_logStreams = 0;

// Copies a BWD file from the buffer passed in from the shell
static void* CopyLaunchBwd(const char* p_name, int32_t* p_size)
{
	const BwdBuffer* bwd;
	void* data;
	uint32_t i;

	if (g_missionLaunch == NULL) {
		return NULL;
	}

	for (i = 0; i < g_missionLaunch->m_bwdCount; i++) {
		bwd = &g_missionLaunch->m_bwds[i];
		if (strcasecmp(bwd->m_name, p_name) == 0) {
			data = MechHeapAlloc(g_primaryHeap, bwd->m_size);
			if (data != NULL) {
				memcpy(data, bwd->m_data, bwd->m_size);
				*p_size = bwd->m_size;
			}
			return data;
		}
	}

	return NULL;
}

// Opens the BWD stream p_key names into p_stream: from a file when the key has no resource id
// (a name without an extension gets ".BWD"), or always with g_streamsFromFiles, and otherwise, or
// when there is no such file, from the resource file. Returns p_stream, or NULL.
BwdStream* OpenBwdStream(BwdStreamKey* p_key, BwdStream* p_stream)
{
	char line[256];
	int32_t size;
	BwdStream* result;
	int32_t file;
	char* dot;
	BwdHeader* header;
	int32_t id;
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

		data = CopyLaunchBwd(p_key->m_name, &size);
		if (data == NULL) {
			file = LoadFile(BuildGamePath(p_key->m_name), &size, &data, NULL);
			if (file != -1) {
				MechClose(file);
			}
			else {
				if (!g_streamsFromFiles) {
					Error(0x29, "%s ID %d", p_key->m_name, p_key->m_id, 0);
				}

				data = NULL;
			}
		}
	}

	if (!data && id != -1) {
		p_stream->m_fromResource = 1;
		p_stream->m_id = id;
		data = LoadCachedResource(g_mw2PrjHandle, id, g_resourceTypeTags[c_resTagBwd]);
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
BwdNode* GetNextNode(BwdStream* p_stream)
{
	BwdNode* next;
	int32_t size;
	BwdNode* result;

	result = NULL;
	if (p_stream->m_current->m_type == g_bwdTypeCodes[0]) {
		size = sizeof(BwdHeader);
	}
	else {
		size = p_stream->m_current->m_size;
	}

	next = (BwdNode*) ((uint8_t*) p_stream->m_current + size);
	if ((uint8_t*) next - (uint8_t*) p_stream->m_data < p_stream->m_size) {
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
char* GetKeywordName(uint32_t p_code)
{
	int32_t i;

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
void LogDebugLine(char* p_text)
{
	// We send it to the Rust side to log with tracing instead
	MechLogDebugf("mw2debug.txt: %s", p_text);

	MonoPrint(p_text);
}

// Logs a keyword's name with LogDebugLine.
void LogKeywordName(uint32_t p_code)
{
	char line[256];
	char* name;

	name = GetKeywordName(p_code);
	sprintf(line, "%s, ", name);
	LogDebugLine(line);
}
