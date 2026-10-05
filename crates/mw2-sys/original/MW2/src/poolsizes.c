#include "poolsizes.h"

#include "bwd.h"
#include "bwdkeywords.h"
#include "bwdnames.h"
#include "bwdrecord.h"
#include "bwdstreamkey.h"
#include "callbacks.h"
#include "decomp.h"
#include "error.h"
#include "geocache.h"
#include "includerecord.h"
#include "includerecord2.h"
#include "mechclass.h"
#include "object.h"
#include "objectanim.h"
#include "players.h"
#include "playersteering.h"
#include "reel.h"
#include "resource.h"
#include "scenariotable.h"
#include "types.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

DECOMP_SIZE_ASSERT(StaticPoolSize, 0x08)

// A mission stream's first record: the BWD version it was written for.
typedef struct RevRecord {
	BwdRecord m_header; // 0x00
	MechU32 m_version;  // 0x08 — four characters, "1.22"
} RevRecord;

// A mission stream's data table: the counts its static memory pools are sized for.
typedef struct DtblRecord {
	BwdRecord m_header;       // 0x00
	MechS16 m_players;        // 0x08
	MechS16 m_unk0x0a;        // 0x0a
	MechS16 m_objects;        // 0x0c
	MechS16 m_classEntries;   // 0x0e
	MechS16 m_unk0x10;        // 0x10
	MechS16 m_unk0x12;        // 0x12
	MechS16 m_anims;          // 0x14
	MechS16 m_animTracks;     // 0x16
	MechS32 m_animFrameBytes; // 0x18
} DtblRecord;

// A four-character tag read as a big-endian number, so that versions compare in order.
#define SWAP_TAG(p_tag)                                                                                                \
	((((p_tag) >> 24) & 0xff) | (((p_tag) << 8) & 0xff0000) | (((p_tag) >> 8) & 0xff00) | ((p_tag) << 24))

// The pool's block tags, four characters each: players (AGP), mechs (MGP), SEG, TLIS, ADAT, ANTK,
// ANFL, OBJI, CID and CINS.
// GLOBAL: MW2 0x100a9428
MechU32 g_staticPoolTags[10] =
	{0x504741, 0x50474d, 0x474553, 0x53494c54, 0x54414441, 0x4b544e41, 0x4c464e41, 0x494a424f, 0x444943, 0x534e4943};

// The counts of a mission's data tables (and of the streams it includes), which size its static
// memory pools.

// GLOBAL: MW2 0x100e9da8
MechS16 g_missionPlayers;

// GLOBAL: MW2 0x100e9daa
MechS16 g_unk0x100e9daa;

// GLOBAL: MW2 0x100e9dac
MechS16 g_missionObjects;

// GLOBAL: MW2 0x100e9dae
MechS16 g_missionClassEntries;

// GLOBAL: MW2 0x100e9db0
MechS16 g_unk0x100e9db0;

// GLOBAL: MW2 0x100e9db2
MechS16 g_unk0x100e9db2;

// GLOBAL: MW2 0x100e9db4
MechS16 g_missionAnims;

// GLOBAL: MW2 0x100e9db6
MechS16 g_missionAnimTracks;

// GLOBAL: MW2 0x100e9db8
MechS32 g_missionAnimFrameBytes;

// The mission's static memory table, which ReadStaticMemoryTable fills.
// GLOBAL: MW2 0x100e9dc0
StaticPoolSize g_staticPoolSizes[10];

// Returns the size of the table's entry at an index, or 0.
// FUNCTION: MW2 0x100563d0
MechU32 GetStaticPoolSize(MechS32 p_index)
{
	MechU32 size;

	size = 0;
	if (p_index >= 0 && p_index < 10) {
		size = g_staticPoolSizes[p_index].m_size;
	}

	return size;
}

// Reads a mission's static memory table (seven tag and size pairs), or returns NULL. A mission
// named by a number is looked up by that resource id.
// Stack-slot permutation: result, key, keyData and buffer.
// FUNCTION: MW2 0x1005640e
StaticPoolSize* ReadStaticMemoryTable(char* p_mission)
{
	BwdStream* stream;
	BwdStreamKey* key;
	StaticPoolSize* result;
	BwdStreamKey keyData;
	undefined buffer[0x20];

	result = NULL;
	key = &keyData;
	strncpy(key->m_name, p_mission, 0xc);
	key->m_name[0xc] = '\0';
	if (isdigit(*p_mission)) {
		key->m_id = atoi(p_mission);
	}
	else {
		key->m_id = -1;
	}

	stream = OpenBwdStream(key, (BwdStream*) buffer);
	if (stream) {
		if (CountMissionStream(stream)) {
			result = BuildStaticMemoryTable();
		}

		UnloadResource(stream);
		FreeBwdNames();
		FreeMissionTables();
	}

	return result;
}

// Reads a mission stream: checks its version, adds its data table's counts (a stream read for the
// first time adds all of them), and runs its scenario table and the streams it includes. Returns
// FALSE on an error.
// The data table's tag test compares in the other operand order, the REV record's byte swap
// takes its terms in another order (the original moves the top byte in with mov al, cl), and
// rev, node, table, known and type are a stack-slot permutation.
// FUNCTION: MW2 0x10056503
MechS32 CountMissionStream(BwdStream* p_stream)
{
	MechS32 result;
	RevRecord* rev;
	BwdNode* node;
	DtblRecord* table;
	MechS32 known;
	MechU32 type;

	result = TRUE;
	rev = (RevRecord*) (node = GetNextNode(p_stream));
	if (!rev || g_bwdTypeCodes[1] != rev->m_header.m_tag) {
		result = FALSE;
		Error(0x48, NULL);
	}
	else if (SWAP_TAG(rev->m_version) < SWAP_TAG(*(MechU32*) g_bwdVersion)) {
		result = FALSE;
		Error(0x49, NULL);
	}
	else {
		node = GetNextNode(p_stream);
		if (!node || node->m_type != g_bwdTypeCodes[2]) {
			result = FALSE;
			Error(0x3e, NULL);
		}
		else {
			if (FindBwdName((BwdName*) &p_stream->m_id)) {
				known = TRUE;
			}
			else {
				known = FALSE;
				AddBwdName((BwdName*) &p_stream->m_id);
			}

			table = (DtblRecord*) node;
			g_missionPlayers += table->m_players;
			g_unk0x100e9daa += table->m_unk0x0a;
			g_missionObjects += table->m_objects;
			g_missionClassEntries += table->m_classEntries;
			g_unk0x100e9db0 += table->m_unk0x10;
			g_unk0x100e9db2 += table->m_unk0x12;
			g_missionAnims += table->m_anims;
			if (!known) {
				g_missionAnimTracks += table->m_animTracks;
				g_missionAnimFrameBytes += table->m_animFrameBytes;
			}

			while (node) {
				type = node->m_type;
				if (g_logStreams) {
					LogKeywordName(type);
				}

				if (g_bwdTypeCodes[11] == type) {
					result &= LoadScenarioTable((ScenarioTable*) node);
				}
				else if (g_bwdTypeCodes[50] == type) {
					result &= ExecuteInclude((IncludeRecord*) node, CountMissionStream);
				}
				else if (g_bwdTypeCodes[52] == type) {
					result &= RunIncludedStream((IncludeRecord2*) node, CountMissionStream);
				}

				node = GetNextNode(p_stream);
			}
		}
	}

	return result;
}

// Fills the static memory table from the mission's counts: each pool's tag and size. The original
// wrote the sizes of Player and PlayerSteering, ReelMotion, Reel and GeoClass as numbers.
// FUNCTION: MW2 0x100567ed
StaticPoolSize* BuildStaticMemoryTable(void)
{
	g_staticPoolSizes[0].m_size = g_missionPlayers * (sizeof(Player) + sizeof(PlayerSteering));
	g_staticPoolSizes[0].m_tag = g_staticPoolTags[0];
	g_staticPoolSizes[1].m_size = GetMechAllocSize() * g_missionPlayers;
	g_staticPoolSizes[1].m_tag = g_staticPoolTags[1];
	g_staticPoolSizes[2].m_size = GetObjSize() * g_missionObjects;
	g_staticPoolSizes[2].m_tag = g_staticPoolTags[2];
	g_staticPoolSizes[3].m_size = GetTimedCallbackSize() * g_missionAnims;
	g_staticPoolSizes[3].m_tag = g_staticPoolTags[3];
	g_staticPoolSizes[4].m_size = g_missionAnims * GetReelMotionSize();
	g_staticPoolSizes[4].m_tag = g_staticPoolTags[4];
	g_staticPoolSizes[5].m_size = g_missionAnimTracks * sizeof(Reel);
	g_staticPoolSizes[5].m_tag = g_staticPoolTags[5];
	g_staticPoolSizes[6].m_size = g_missionAnimFrameBytes;
	g_staticPoolSizes[6].m_tag = g_staticPoolTags[6];
	g_staticPoolSizes[7].m_size = g_missionClassEntries * sizeof(GeoClass);
	g_staticPoolSizes[7].m_tag = g_staticPoolTags[7];
	g_staticPoolSizes[8].m_size = g_missionClassEntries * 4;
	g_staticPoolSizes[8].m_tag = g_staticPoolTags[8];
	g_staticPoolSizes[9].m_size = g_missionClassEntries * 4;
	g_staticPoolSizes[9].m_tag = g_staticPoolTags[9];
	return g_staticPoolSizes;
}
