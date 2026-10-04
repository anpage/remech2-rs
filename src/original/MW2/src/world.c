#include "world.h"

#include "anim2d.h"
#include "animation.h"
#include "bwd.h"
#include "bwdblockrecord.h"
#include "bwdkeywords.h"
#include "bwdobjectrecord.h"
#include "bwdrecord.h"
#include "bwdstreamkey.h"
#include "callbacks.h"
#include "classtable.h"
#include "cockpit.h"
#include "collision.h"
#include "config.h"
#include "decomp.h"
#include "effect.h"
#include "environment.h"
#include "error.h"
#include "eyepoint.h"
#include "faceshade.h"
#include "fixedmul.h"
#include "gamething.h"
#include "geocache.h"
#include "gridobject.h"
#include "includerecord.h"
#include "includerecord2.h"
#include "maneuvers.h"
#include "mech.h"
#include "mekfile.h"
#include "missionsetup.h"
#include "missiontable.h"
#include "navpoint.h"
#include "network.h"
#include "object.h"
#include "objectanim.h"
#include "palette.h"
#include "players.h"
#include "playertype.h"
#include "polydraw.h"
#include "render.h"
#include "resource.h"
#include "resourcename.h"
#include "resourceref.h"
#include "scenariotable.h"
#include "shape.h"
#include "shots.h"
#include "simmain.h"
#include "soundfx.h"
#include "targeting.h"
#include "targetpanel.h"
#include "team.h"
#include "types.h"
#include "vector3.h"
#include "wtbshapes.h"
#include "xform.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)

// The world stream's records BwdExecuteStream reads, after the BwdRecord header. Only the members
// it reads are laid out.

// planet: the gravity, the time of day and climate, and render switches.
typedef struct BwdPlanetRecord {
	BwdRecord m_header;       // 0x00
	MechS32 m_gravity;        // 0x08 — in g
	undefined4 m_unk0x0c;     // 0x0c
	MechS32 m_daysPerYear;    // 0x10
	MechS32 m_secondsPerDay;  // 0x14
	undefined4 m_unk0x18[3];  // 0x18
	MechS32 m_temperature;    // 0x24
	MechS32 m_jumpJetDrag;    // 0x28
	MechS32 m_mapShadeTop;    // 0x2c
	MechS32 m_mapShadeBase;   // 0x30
	MechS32 m_slideSlope;     // 0x34
	MechS32 m_breathable;     // 0x38
	MechS32 m_brightenDamage; // 0x3c
	MechS32 m_noEffectCamera; // 0x40
	MechS32 m_noSky;          // 0x44
	MechS32 m_noGround;       // 0x48
	MechS32 m_noHorizonBand;  // 0x4c
} BwdPlanetRecord;

// The second planet record (c_bwdClimate).
typedef struct BwdClimateRecord {
	BwdRecord m_header;   // 0x00
	undefined4 m_unk0x08; // 0x08
	MechS16 m_unk0x0c;    // 0x0c
} BwdClimateRecord;

// hiddentext.
typedef struct BwdTextRecord {
	BwdRecord m_header; // 0x00
	MechChar m_text[1]; // 0x08
} BwdTextRecord;

// A record of two values.
typedef struct BwdPairRecord {
	BwdRecord m_header; // 0x00
	MechS32 m_a;        // 0x08
	MechS32 m_b;        // 0x0c
} BwdPairRecord;

// A record of a resource: luma, music, animsound, bitmap prj and id list, the cockpit files.
typedef struct BwdResourceRecord {
	BwdRecord m_header; // 0x00
	ResourceRef m_ref;  // 0x08
} BwdResourceRecord;

// now: the time of day.
typedef struct BwdNowRecord {
	BwdRecord m_header;      // 0x00
	undefined4 m_unk0x08[2]; // 0x08
	MechS32 m_timeOfDay;     // 0x10
	MechS16 m_dayOfYear;     // 0x14
} BwdNowRecord;

// palettegroup: the palettes of the 20 slots.
typedef struct BwdPaletteGroupRecord {
	BwdRecord m_header; // 0x00
	MechS16 m_ids[20];  // 0x08
} BwdPaletteGroupRecord;

// light: the eyepoint's light.
typedef struct BwdLightRecord {
	BwdRecord m_header;      // 0x00
	undefined4 m_unk0x08[2]; // 0x08
	MechS32 m_x;             // 0x10
	MechS32 m_y;             // 0x14
	MechS32 m_z;             // 0x18
	MechS16 m_ambient;       // 0x1c
	MechS16 m_directional;   // 0x1e
	MechS32 m_fadeDistance;  // 0x20
} BwdLightRecord;

// A window record's rectangle for one of g_panes.
typedef struct BwdWindowEntry {
	MechS32 m_index;         // 0x00
	MechS32 m_x;             // 0x04
	MechS32 m_y;             // 0x08
	MechS32 m_width;         // 0x0c
	MechS32 m_height;        // 0x10
	undefined4 m_unk0x14[3]; // 0x14
} BwdWindowEntry;

// window: rectangles for the panes, as many as fit the record.
typedef struct BwdWindowRecord {
	BwdRecord m_header;          // 0x00
	BwdWindowEntry m_entries[1]; // 0x08
} BwdWindowRecord;

// start: the eyepoint's view.
typedef struct BwdStartRecord {
	BwdRecord m_header;  // 0x00
	MechS32 m_values[7]; // 0x08 — Eyepoint 0x00-0x18
} BwdStartRecord;

// A record of two resource numbers: bitmapsection, bitmapenable.
typedef struct BwdIdPairRecord {
	BwdRecord m_header; // 0x00
	MechS16 m_a;        // 0x08
	MechS16 m_b;        // 0x0a
} BwdIdPairRecord;

// frameprj.
typedef struct BwdFramePrjRecord {
	BwdRecord m_header; // 0x00
	MechS16 m_unk0x08;  // 0x08 — -1 for 1
	MechS16 m_unk0x0a;  // 0x0a — -1 for 0
	MechS16 m_id;       // 0x0c — -1 for none
} BwdFramePrjRecord;

// polyoffset: the shape offset.
typedef struct BwdOffsetRecord {
	BwdRecord m_header; // 0x00
	MechS32 m_x;        // 0x08
	MechS32 m_y;        // 0x0c
	MechS32 m_z;        // 0x10
} BwdOffsetRecord;

// blockxform.
typedef struct BwdXformRecord {
	BwdRecord m_header; // 0x00
	Xform m_xform;      // 0x08
} BwdXformRecord;

// A record of an object class or thing, and a value: scrounge, thing, gamepiece, eyeobj, pof,
// objloc, booyowthing, xplode, lightobj. Only xplode records carry m_effectType.
typedef struct BwdIdRecord {
	BwdRecord m_header;   // 0x00
	MechS16 m_id;         // 0x08
	MechS16 m_value;      // 0x0a
	MechS16 m_effectType; // 0x0c
} BwdIdRecord;

// gamething: an object (and its replacement) that counts for the mission.
typedef struct BwdGameThingRecord {
	BwdRecord m_header;         // 0x00
	MechS16 m_objects[4][2];    // 0x08 — the object and its replacement
	MechS16 m_hitPoints;        // 0x18
	MechU16 m_events;           // 0x1a
	MechS32 m_flags;            // 0x1c
	MechChar m_name[0x16];      // 0x20
	MechChar m_shortName[0x16]; // 0x36
} BwdGameThingRecord;

// navpoint.
typedef struct BwdNavPointRecord {
	BwdRecord m_header;    // 0x00
	Vector3 m_position;    // 0x08
	MechS32 m_heading;     // 0x14
	MechS16 m_used;        // 0x18
	MechS16 m_flags;       // 0x1a
	MechS16 m_owner;       // 0x1c
	MechS16 m_team;        // 0x1e
	MechU16 m_radius;      // 0x20 — in hundreds
	MechU16 m_events;      // 0x22
	MechChar m_name[0x16]; // 0x24
} BwdNavPointRecord;

// navobject: a nav point on an object.
typedef struct BwdNavObjectRecord {
	BwdRecord m_header; // 0x00
	MechS16 m_unk0x08;  // 0x08
	MechS16 m_id;       // 0x0a
	MechS16 m_radius;   // 0x0c
	MechU16 m_events;   // 0x0e
} BwdNavObjectRecord;

// task: a timed callback, on an object or detached.
typedef struct BwdTaskRecord {
	BwdRecord m_header; // 0x00
	MechS16 m_kind;     // 0x08 — an index into g_taskFns
	MechS32 m_period;   // 0x0a
	MechChar m_data[1]; // 0x0e — "object;..."
} BwdTaskRecord;

// position and rotate: an object class's instance, placed.
typedef struct BwdPlaceRecord {
	BwdRecord m_header; // 0x00
	MechS16 m_id;       // 0x08
	MechS32 m_x;        // 0x0a
	MechS32 m_y;        // 0x0e
	MechS32 m_z;        // 0x12
} BwdPlaceRecord;

// anim2d's parameters (LoadAnim2d).
typedef struct BwdAnim2dSpec {
	MechS16 m_flags;      // 0x00
	MechS16 m_frameTime;  // 0x02
	MechS16 m_type;       // 0x04
	MechS16 m_resourceId; // 0x06
} BwdAnim2dSpec;

// anim2d.
typedef struct BwdAnim2dRecord {
	BwdRecord m_header;   // 0x00
	BwdAnim2dSpec m_spec; // 0x08
} BwdAnim2dRecord;

#pragma pack(pop)

// Set by the local player's gpspec record: its hudfile record loads the HUD once.
// GLOBAL: MW2 0x100a173c
MechS32 g_loadHudFile = 0;

// The next g_shots slot a booyowthing record fills.
// GLOBAL: MW2 0x100a1740
MechS32 g_nextShotRecord = 0;

// The next g_effects slot an xplode record fills.
// GLOBAL: MW2 0x100a1744
MechS32 g_nextEffectRecord = 0;

// The name of the mission's music (the world stream's music record).
// GLOBAL: MW2 0x100e9330
MechChar g_musicName[12];

// The mission's "MUS" resource.
// GLOBAL: MW2 0x100e9340
MechS32 g_musicResource;

// Widens p_flags: any of the bits 0x730 sets them all, as does either of the bits 3.
// FUNCTION: MW2 0x1000a9c0
MechU32 WidenEventFlags(MechU32 p_flags)
{
	if (p_flags & 0x730) {
		p_flags |= 0x730;
	}

	if (p_flags & 3) {
		p_flags |= 3;
	}

	return p_flags;
}

// Executes a BWD stream: the world's scripts, objects and things, record by record. Include,
// gamepiece and cockpit file records execute their own streams. Events of the things and nav
// points it creates are posted to the stream's event list (the mission table's, once one is
// read). Returns whether every stream it executed loaded.
// Stack-slot permutation of the locals (its wider [ebp-N] encodings also shift the jumps).
// Operand order: the rep record's i < g_thingCapacity loads g_thingCapacity first in the original.
// FUNCTION: MW2 0x1000a9f5
MechS32 BwdExecuteStream(BwdStream* p_stream)
{
	MechS32 result;
	BwdNode* node;
	MechU32 eventList;
	MechU32 type;
	MechChar* eventName;
	undefined4 unk0x18;
	MechS32 affiliation;

	result = TRUE;
	eventList = 0;
	unk0x18 = 0;
	affiliation = -1;
	eventList = FindEventList(p_stream->m_name);
	eventName = p_stream->m_name;

	while ((node = GetNextNode(p_stream)) != NULL) {
		type = node->m_type;
		if (type == g_bwdTypeCodes[c_bwdPlanet]) {
			BwdPlanetRecord* planet = (BwdPlanetRecord*) node;

			if (g_difficulty->m_gravity) {
				g_gravity = FixedMul16(g_difficulty->m_gravity, 0x794);
			}
			else {
				g_gravity = FixedMul16(planet->m_gravity, 0x794);
			}

			g_daysPerYear = planet->m_daysPerYear;
			g_secondsPerDay = planet->m_secondsPerDay;
			if (g_difficulty->m_temperature) {
				g_temperature = g_difficulty->m_temperature;
			}
			else {
				g_temperature = planet->m_temperature;
			}

			g_hostileAtmosphere = planet->m_breathable == 0;
			g_brightenDamage = planet->m_brightenDamage;
			g_effectCameraEnabled = planet->m_noEffectCamera == 0;
			g_renderSettings.m_drawSky = planet->m_noSky == 0;
			g_renderSettings.m_drawGround = planet->m_noGround == 0;
			g_renderSettings.m_horizonBand = planet->m_noHorizonBand == 0;
			if (planet->m_jumpJetDrag > 0) {
				g_jumpJetDrag = planet->m_jumpJetDrag;
			}

			if (planet->m_mapShadeTop || planet->m_mapShadeBase) {
				g_mapShadeTop = planet->m_mapShadeTop;
				g_mapShadeBase = planet->m_mapShadeBase;
			}

			if (planet->m_slideSlope > 0) {
				g_slideSlope = planet->m_slideSlope;
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdClimate]) {
			BwdClimateRecord* climate = (BwdClimateRecord*) node;

			g_unk0x100ba608 = climate->m_unk0x0c;
		}
		else if (type == g_bwdTypeCodes[c_bwdHiddenText]) {
			BwdTextRecord* text = (BwdTextRecord*) node;

			strcpy(g_anonymousInstallationName, text->m_text);
		}
		else if (type == g_bwdTypeCodes[c_bwdView]) {
			BwdPairRecord* view = (BwdPairRecord*) node;

			g_mainEyepoint.m_nearPlane = view->m_a;
			g_mainEyepoint.m_farPlane = view->m_b;
		}
		else if (type == g_bwdTypeCodes[c_bwdLuma]) {
			MechS32 id;
			BwdResourceRecord* luma = (BwdResourceRecord*) node;

			id = luma->m_ref.m_id;
			if (id == -1) {
				id = FindResourceIdByName(0xd, luma->m_ref.m_name);
			}

			if (id != -1) {
				g_lumaResourceId = id;
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdMusic]) {
			BwdResourceRecord* music = (BwdResourceRecord*) node;

			g_musicResource = music->m_ref.m_id;
			if (g_musicResource == -1) {
				g_musicResource = FindResourceIdByName(0xc, music->m_ref.m_name);
			}

			strcpy(g_musicName, music->m_ref.m_name);
		}
		else if (type == g_bwdTypeCodes[c_bwdAnimSound]) {
			BwdResourceRecord* sound = (BwdResourceRecord*) node;

			if (g_lastPlayer && !g_lastPlayer->m_aiMode) {
				g_lastPlayer->m_pendingSound = sound->m_ref.m_id;
				if (g_lastPlayer->m_pendingSound == -1) {
					g_lastPlayer->m_pendingSound = FindResourceIdByName(0xb, sound->m_ref.m_name);
				}
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdAffiliation]) {
			BwdPairRecord* affil = (BwdPairRecord*) node;

			affiliation = affil->m_a;
		}
		else if (type == g_bwdTypeCodes[c_bwdTerrain]) {
		}
		else if (type == g_bwdTypeCodes[c_bwdNow]) {
			BwdNowRecord* now = (BwdNowRecord*) node;

			if (g_isNetworkGame && g_difficulty) {
				g_timeOfDayPhase = g_difficulty->m_timeOfDay;
			}
			else {
				g_timeOfDayPhase = now->m_timeOfDay;
			}

			g_dayOfYear = now->m_dayOfYear;
		}
		else if (type == g_bwdTypeCodes[c_bwdPaletteGroup]) {
			MechS32 slot;
			MechS32 i;
			BwdPaletteGroupRecord* group = (BwdPaletteGroupRecord*) node;
			MechS16* ids;

			slot = 0;
			ids = group->m_ids;
			for (i = 0; i < 4; i++) {
				SetPaletteResourceId(ids[0], slot);
				SetPaletteResourceId(ids[1], slot + 1);
				SetPaletteResourceId(ids[2], slot + 2);
				SetPaletteResourceId(ids[3], slot + 3);
				slot += 4;
				ids += 4;
			}

			SetPaletteResourceId(group->m_ids[16], 0x10);
			SetPaletteResourceId(group->m_ids[17], 0x11);
			SetPaletteResourceId(group->m_ids[18], 0x12);
			SetPaletteResourceId(group->m_ids[19], 0x13);
		}
		else if (type == g_bwdTypeCodes[c_bwdLight]) {
			BwdLightRecord* light = (BwdLightRecord*) node;

			g_mainEyepoint.m_lightX = light->m_x;
			g_mainEyepoint.m_lightY = light->m_y;
			g_mainEyepoint.m_lightZ = light->m_z;
			g_mainEyepoint.m_directionalLight = light->m_directional;
			g_eyepoint->m_ambientLight = light->m_ambient;
			g_renderSettings.m_fadeDistance = light->m_fadeDistance;
			if (g_renderSettings.m_fadeDistance > 0) {
				g_renderSettings.m_distanceFade = 1;
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdGroundMap]) {
			LoadMapBitmap((BwdRecord*) node, &g_groundColor, NULL, NULL);
		}
		else if (type == g_bwdTypeCodes[c_bwdHorizonMap]) {
			LoadMapBitmap((BwdRecord*) node, &g_horizonMapColor, &g_horizonBandHeight, NULL);
		}
		else if (type == g_bwdTypeCodes[c_bwdSkyMap]) {
			LoadMapBitmap((BwdRecord*) node, &g_skyColor, NULL, NULL);
		}
		else if (type == g_bwdTypeCodes[c_bwdWindow]) {
			MechS32 count;
			BwdWindowEntry* entry;
			MechS32 i;
			MechS32 index;
			BwdWindowRecord* window = (BwdWindowRecord*) node;

			entry = window->m_entries;
			count = (window->m_header.m_size - 8) / sizeof(BwdWindowEntry);
			if (count < 0 || count > 8) {
				Error(0x27, NULL);
			}
			else {
				for (i = 0; i < count; i++) {
					index = entry->m_index;
					if (index <= 8 && index >= 0) {
						g_panes[index].m_x0 = entry->m_x;
						g_panes[index].m_y0 = entry->m_y;
						g_panes[index].m_x1 = entry->m_x + entry->m_width - 1;
						g_panes[index].m_y1 = entry->m_y + entry->m_height - 1;
					}
					else {
						Error(0x27, NULL);
					}

					entry++;
				}
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdStart]) {
			BwdStartRecord* start = (BwdStartRecord*) node;

			g_mainEyepoint.m_x = start->m_values[0];
			g_mainEyepoint.m_y = start->m_values[1];
			g_mainEyepoint.m_z = start->m_values[2];
			g_mainEyepoint.m_heading = start->m_values[3];
			g_mainEyepoint.m_pitch = start->m_values[4];
			g_mainEyepoint.m_roll = start->m_values[5];
			g_mainEyepoint.m_fovX = start->m_values[6];
		}
		else if (type == g_bwdTypeCodes[c_bwdScenarioTable]) {
			LoadScenarioTable((ScenarioTable*) node);
		}
		else if (type == g_bwdTypeCodes[c_bwdStar]) {
			LoadStarTable((struct StarTable*) node);
		}
		else if (type == g_bwdTypeCodes[c_bwdPath]) {
			LoadPathTable((struct PathRecord*) node);
		}
		else if (type == g_bwdTypeCodes[c_bwdFormation]) {
			LoadFormationTable((struct FormationRecord*) node);
		}
		else if (type == g_bwdTypeCodes[c_bwdMissionTable]) {
			LoadMissionTable((MissionTable*) node);
			eventList = FindEventList("$");
			eventName = "$";
		}
		else if (type == g_bwdTypeCodes[c_bwdBitmapPrj]) {
			BwdResourceRecord* prj = (BwdResourceRecord*) node;
			MechS32 id;

			id = prj->m_ref.m_id;
			if (id == -1) {
				id = FindResourceIdByName(8, prj->m_ref.m_name);
			}

			if (id != -1) {
				AddAnimFrame(id, -1);
			}
			else {
				Error(0x28, NULL);
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdBitmapIdList]) {
			BwdResourceRecord* list = (BwdResourceRecord*) node;
			MechS32 id;

			id = list->m_ref.m_id;
			StartAnimation(id, -1);
		}
		else if (type == g_bwdTypeCodes[c_bwdBitmapSection]) {
			BwdIdPairRecord* section = (BwdIdPairRecord*) node;
			MechS16 delay;
			MechS16 index;

			index = section->m_a;
			delay = section->m_b;
			SetAnimDelay(index, delay);
			SetAnimMode(index, 1);
		}
		else if (type == g_bwdTypeCodes[c_bwdBitmapEnable]) {
			BwdIdPairRecord* enable = (BwdIdPairRecord*) node;
			MechS16 mode;
			MechS16 index;

			index = enable->m_a;
			mode = enable->m_b;
			SetAnimMode(index, mode);
		}
		else if (type == g_bwdTypeCodes[c_bwdFramePrj]) {
			MechS16 b;
			MechS16 id;
			BwdFramePrjRecord* frame = (BwdFramePrjRecord*) node;
			MechS16 a;

			a = frame->m_unk0x08;
			b = frame->m_unk0x0a;
			id = frame->m_id;
			if (a == -1) {
				a = 1;
			}

			if (b == -1) {
				b = 0;
			}

			if (id != -1) {
				LoadFramePrj(id, a, b);
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdPolyOffset]) {
			MechS32 x;
			MechS32 y;
			MechS32 z;
			BwdOffsetRecord* offset = (BwdOffsetRecord*) node;

			x = offset->m_x;
			y = offset->m_y;
			z = offset->m_z;
			SetShapeOffset(x, y, z);
		}
		else if (type == g_bwdTypeCodes[c_bwdBlockXform]) {
			BwdXformRecord* xform = (BwdXformRecord*) node;

			ApplyBlockXform(xform->m_xform);
		}
		else if (type == g_bwdTypeCodes[c_bwdRep]) {
			MechS32 i;

			if (!g_inRepeat) {
				g_inRepeat = 1;
				g_repeatPass = 0;
				g_thingCount = 0;
				for (i = 0; i < g_thingCapacity; i++) {
					g_thingIndices[i] = -1;
				}
			}
			else {
				g_repeatPass++;
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdEndRep]) {
			g_inRepeat = 0;
		}
		else if (type == g_bwdTypeCodes[c_bwdBlock]) {
			BeginBlock((BwdBlockRecord*) node);
		}
		else if (type == g_bwdTypeCodes[c_bwdElseBlock]) {
			HandleElseBlock();
		}
		else if (type == g_bwdTypeCodes[c_bwdEndBlock]) {
			EndBlock(p_stream);
		}
		else if (type == g_bwdTypeCodes[c_bwdObject]) {
			CreateObjectNode((BwdObjectRecord*) node, unk0x18, 0, g_blockDepth, g_inRepeat, g_repeatPass);
		}
		else if (type == g_bwdTypeCodes[c_bwdAnimFile]) {
			ResourceRef* ref;
			BwdResourceRecord* anim = (BwdResourceRecord*) node;

			ref = &anim->m_ref;
			if (!LoadAnimFile(ref)) {
				Error(0x4f, NULL);
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdScrounge]) {
			SceneObject* obj;
			Shape* shape;
			MechS32 id;
			BwdIdRecord* scrounge = (BwdIdRecord*) node;

			id = scrounge->m_id;
			id = MapResourceId(id);
			shape = FindClassById(id);
			if (shape && (obj = GetShapeObject(shape)) != NULL) {
				SetGridObject(obj);
			}
			else {
				Error(0x2e, NULL);
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdThing]) {
			MechS32 index;
			MechS32 id;
			BwdIdRecord* thing = (BwdIdRecord*) node;

			id = thing->m_id;
			id = MapResourceId(id);
			if (g_thingRecordCount < 0x96) {
				index = FindThingIdxById(id);
				if (index != -1) {
					g_thingRecordIndices[g_thingRecordCount] = index;
					g_thingRecordCount++;
				}
				else {
					Error(0x2f, NULL);
				}
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdGamepiece]) {
			MechS32 kind;
			Player* player;
			PlayerType* playerType;
			MechS32 typeIndex;
			MechS32 i;
			BwdIdRecord* gamepiece = (BwdIdRecord*) node;
			SceneObject* obj;
			Shape* shape;
			MechS32 id;

			typeIndex = -1;
			obj = NULL;
			id = gamepiece->m_id;
			kind = gamepiece->m_value;
			if (g_playerCount >= 0x3c) {
				break;
			}

			id = MapResourceId(id);
			if (kind >= 0 && kind <= 8) {
				shape = FindClassById(id);
				if (shape) {
					obj = GetShapeObject(shape);
					if (obj) {
						for (i = 0; i <= 8; i++) {
							if (g_playerTypes[i].m_type == kind) {
								typeIndex = i;
								break;
							}
						}

						if (typeIndex == -1) {
							Error(0x30, NULL);
							typeIndex = 0;
						}

						playerType = &g_playerTypes[typeIndex];
						CreateSimPlayer(g_playerCount, playerType->m_create);
						player = g_players[g_playerCount];
						if (!player) {
							Error(0xd, NULL);
						}
						else if (!player->m_mech) {
							Error(0xe, NULL);
						}

						player->m_type = playerType->m_type;
						player->m_firstClassFn = playerType->m_firstClassFn;
						player->m_updateFn = playerType->m_updateFn;
						player->m_lateUpdateFn = playerType->m_lateUpdateFn;
						player->m_shutdownFn = playerType->m_shutdownFn;
						player->m_baseLevel = g_repeatPass;
						player->m_detailLevel = -1;
						if (g_localPlayerId == g_playerCount) {
							player->m_aiMode = 0;
							player->m_localUpdateFn = playerType->m_localUpdateFn;
							player->m_drawFn = playerType->m_drawFn;
						}
						else {
							player->m_localUpdateFn = NULL;
							player->m_drawFn = NULL;
						}

						player->m_index = g_playerCount;
						g_playerCount++;
						g_lastPlayer = player;
						player->m_obj = obj;
						player->m_eyeObj = obj;
						ClaimNewClassEntries(player);
						LoadBaseLevelShapes(player);
					}
				}
				else {
					g_lastPlayer = NULL;
					player->m_obj = NULL;
				}
			}
			else {
				Error(0x30, NULL);
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdCptFile]) {
			ResourceRef* ref;
			BwdResourceRecord* cockpit = (BwdResourceRecord*) node;

			ref = &cockpit->m_ref;
			if (g_lastPlayer && !g_lastPlayer->m_aiMode) {
				LoadCptFile(ref, g_cockpitGaugePanes, g_cockpitPanelPanes, g_unk0x100a5bb8[3]);
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdPitFile]) {
			BwdStream* stream;
			BwdStreamKey* key;
			BwdStreamKey keyData;
			BwdResourceRecord* pit = (BwdResourceRecord*) node;
			undefined buffer[0x20];

			if (g_lastPlayer && !g_lastPlayer->m_aiMode) {
				key = &keyData;
				key->m_id = pit->m_ref.m_id;
				strcpy(key->m_name, pit->m_ref.m_name);
				stream = OpenBwdStream(key, (BwdStream*) buffer);
				if (stream) {
					result &= BwdExecuteStream(stream);
					UnloadResource(stream);
				}
				else {
					Error(0x31, NULL);
				}
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdVptFile]) {
			BwdStream* stream;
			BwdStreamKey* key;
			BwdStreamKey keyData;
			BwdResourceRecord* vpt = (BwdResourceRecord*) node;
			undefined buffer[0x20];

			if (g_lastPlayer && !g_lastPlayer->m_aiMode) {
				key = &keyData;
				key->m_id = vpt->m_ref.m_id;
				strcpy(key->m_name, vpt->m_ref.m_name);
				stream = OpenBwdStream(key, (BwdStream*) buffer);
				if (stream) {
					result &= BwdExecuteStream(stream);
					UnloadResource(stream);
				}
				else {
					Error(0x32, NULL);
				}
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdHudFile]) {
			ResourceRef* ref;
			BwdResourceRecord* hud = (BwdResourceRecord*) node;

			ref = &hud->m_ref;
			if (g_lastPlayer && g_loadHudFile) {
				LoadHudFile(ref);
				g_loadHudFile = 0;
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdMgdFile]) {
			ResourceRef* ref;
			Mech* mech;
			BwdResourceRecord* mgd = (BwdResourceRecord*) node;

			ref = &mgd->m_ref;
			if (g_lastPlayer) {
				mech = g_lastPlayer->m_mech;
				LoadMgdFile(
					ref,
					&mech->m_height,
					&mech->m_cockpitHeight,
					&mech->m_unk0xd4,
					&mech->m_unk0xd8,
					&mech->m_unk0xdc,
					&mech->m_maxTorsoTwist,
					&mech->m_radius
				);
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdEyeObj]) {
			SceneObject* obj;
			MechS32 index;
			MechS32 id;
			BwdIdRecord* eye = (BwdIdRecord*) node;

			if (g_lastPlayer) {
				id = eye->m_id;
				id = MapResourceId(id);
				index = FindThingIdxById(id);
				if (index >= 0) {
					obj = GetClassObject(index);
					if (obj) {
						g_lastPlayer->m_eyeObj = obj;
					}
				}
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdPof]) {
			Mech* mech;
			BwdIdRecord* pof = (BwdIdRecord*) node;
			SceneObject* obj;
			MechS32 index;
			MechS32 id;

			if (g_lastPlayer) {
				id = pof->m_id;
				id = MapResourceId(id);
				mech = g_lastPlayer->m_mech;
				index = FindThingIdxById(id);
				if (index >= 0) {
					obj = GetClassObject(index);
					if (obj) {
						mech->m_objects[pof->m_value] = obj;
					}
				}
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdGameThing]) {
			MechS32 i;
			MechU32 events;
			GameThing* thing;
			MechS32 index;
			MechS32 replacementIndex;
			MechS32 objectIndex;
			BwdGameThingRecord* gameThing = (BwdGameThingRecord*) node;
			MechS32 hitPoints;
			MechS32 object;
			MechU16 target;
			MechS32 replacement;

			i = 0;
			thing = NULL;
			hitPoints = gameThing->m_hitPoints;
			events = WidenEventFlags(gameThing->m_events);
			index = FindFreeGameThing();
			if (index != -1) {
				thing = &g_gameThings[index];
				thing->m_flags = gameThing->m_flags;
				thing->m_hitPoints = hitPoints;
				thing->m_affiliation = affiliation;
				strncpy(thing->m_name, gameThing->m_name, 0x16);
				thing->m_name[0x15] = '\0';
				strncpy(thing->m_shortName, gameThing->m_shortName, 0x16);
				thing->m_shortName[0x15] = '\0';
				object = gameThing->m_objects[i][0];
				replacement = gameThing->m_objects[i][1];
				objectIndex = -1;
				replacementIndex = -1;
				if (object != -1) {
					object = MapResourceId(object);
					objectIndex = FindStarIdxById(object);
					if (objectIndex == -1) {
						objectIndex = AllocStaticObject(object);
					}

					replacementIndex = -1;
					if (replacement != -1) {
						replacement = MapResourceId(replacement);
						replacementIndex = FindStarIdxById(replacement);
						if (replacementIndex == -1) {
							replacementIndex = AllocStaticObject(replacement);
						}
					}

					if (objectIndex != -1) {
						LinkStaticObjectThing(objectIndex, replacementIndex, index);
					}
				}

				thing->m_staticObject = objectIndex;
				if (eventList) {
					target = (MechU8) index | 0x400;
					PostEventToList(eventName, events, target);
				}
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdObjLoc]) {
			MechS32 index;
			MechS32 value;
			MechS32 id;
			BwdIdRecord* objLoc = (BwdIdRecord*) node;

			id = objLoc->m_id;
			value = objLoc->m_value;
			id = MapResourceId(id);
			if (value != -1) {
				index = FindThingIdxById(id);
				if (index != -1) {
					SetClassEntryPartId(index, value);
				}
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdBooyowThing]) {
			MechS32 shotType;
			BwdIdRecord* booyow = (BwdIdRecord*) node;
			SceneObject* obj;
			Shape* shape;
			MechS32 id;

			id = booyow->m_id;
			shotType = booyow->m_value;
			if (g_nextShotRecord < 0xaf && g_nextShotRecord > -1) {
				id = MapResourceId(id);
				if (!g_shots[g_nextShotRecord].m_object) {
					shape = FindClassById(id);
					if (shape) {
						obj = GetShapeObject(shape);
						if (obj) {
							g_shots[g_nextShotRecord].m_type = shotType;
							g_shots[g_nextShotRecord].m_object = obj;
							g_shots[g_nextShotRecord].m_unk0x3c = 0;
							HideObjTree(obj);
							DisableObjTreeCollision(obj);
							SetObjTreeKind(obj, 0x400);
						}
					}
				}

				g_nextShotRecord++;
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdXplode]) {
			Effect* effect;
			BwdIdRecord* xplode = (BwdIdRecord*) node;
			MechS32 effectType;
			MechS32 animation;
			SceneObject* obj;
			Shape* shape;
			MechS32 id;

			id = xplode->m_id;
			animation = xplode->m_value;
			effectType = xplode->m_effectType;
			if (g_nextEffectRecord >= 0 && g_nextEffectRecord < 0x100) {
				if (effectType < 0 || effectType >= 0x20) {
					effectType = 3;
				}

				effect = &g_effects[g_nextEffectRecord];
				if (!effect->m_object) {
					if (id != -1) {
						id = MapResourceId(id);
						shape = FindClassById(id);
						if (shape) {
							obj = GetShapeObject(shape);
							if (obj) {
								effect->m_object = obj;
								HideObjTree(obj);
								DisableObjTreeCollision(obj);
								if (!(shape->m_kind & 0xf0)) {
									SetObjTreeKind(obj, 0x20);
								}

								effect->m_animation = animation;
								if (animation > 0) {
									SetAnimMode(effect->m_animation, 2);
								}
							}
						}
					}

					effect->m_type = effectType;
					effect->m_active = 0;
				}

				g_nextEffectRecord++;
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdNavPoint]) {
			BwdNavPointRecord* navPoint = (BwdNavPointRecord*) node;
			MechU32 events;
			Vector3 position;
			MechU16 target;
			NavPoint* nav;

			if (g_navCount < 0x80 && g_navCount != -1) {
				nav = &g_navTable[g_navCount];
				position = navPoint->m_position;
				TransformBlockPoint(&position.m_x);
				nav->m_position[0] = position.m_x;
				nav->m_position[1] = position.m_y;
				nav->m_position[2] = position.m_z;
				nav->m_used = navPoint->m_used;
				nav->m_flags = navPoint->m_flags;
				nav->m_team = navPoint->m_team;
				nav->m_owner = navPoint->m_owner;
				nav->m_heading = navPoint->m_heading;
				nav->m_radius = navPoint->m_radius * 100;
				nav->m_obj = NULL;
				strncpy(nav->m_name, navPoint->m_name, 0x15);
				nav->m_name[0x15] = '\0';
				events = WidenEventFlags(navPoint->m_events);
				target = (MechU8) g_navCount | 0x100;
				g_navCount++;
				if (eventList) {
					PostEventToList(eventName, events, target);
				}
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdNavObject]) {
			BwdNavObjectRecord* navObject = (BwdNavObjectRecord*) node;
			SceneObject* obj;
			MechS32 index;
			MechU32 events;
			MechS32 x;
			MechS32 found;
			MechS32 y;
			MechS32 z;
			Shape* shape;
			MechS32 id;
			MechU16 target;

			found = FALSE;
			if (g_navCount < 0x80 && g_navCount != -1) {
				id = navObject->m_id;
				id = MapResourceId(id);
				index = FindStarIdxById(id);
				if (index != -1) {
					GetStaticObjectPosition(index, &x, &y, &z);
					g_navTable[g_navCount].m_position[0] = x;
					g_navTable[g_navCount].m_position[1] = y;
					g_navTable[g_navCount].m_position[2] = z;
					found = TRUE;
				}
				else {
					shape = FindClassById(id);
					if (shape) {
						obj = GetShapeObject(shape);
						if (obj) {
							GetObjWorldAngles(obj, &x, &y, &z);
							g_navTable[g_navCount].m_obj = obj;
							g_navTable[g_navCount].m_heading = y;
							found = TRUE;
						}
					}
				}

				if (found) {
					g_navTable[g_navCount].m_radius = navObject->m_radius;
					g_navTable[g_navCount].m_used = 1;
					g_navTable[g_navCount].m_flags = 0;
					g_navTable[g_navCount].m_owner = 0;
					events = WidenEventFlags(navObject->m_events);
					target = (MechU8) g_navCount | 0x100;
					g_navCount++;
					if (eventList) {
						PostEventToList(eventName, events, target);
					}
				}
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdLightObj]) {
			MechS32 index;
			BwdIdRecord* lightObj = (BwdIdRecord*) node;
			MechS32 id;

			id = lightObj->m_id;
			if (id != -1) {
				id = MapResourceId(id);
				index = FindStarIdxById(id);
				if (index != -1) {
					g_hasLightObject = 1;
					g_lightFollowsObject = 1;
					g_lightObject = index;
				}
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdTask]) {
			MechS32 period;
			MechChar* semicolon;
			MechChar* data;
			MechS32 index;
			MechS32 kind;
			MechS32 objectId;
			BwdTaskRecord* task = (BwdTaskRecord*) node;

			objectId = -1;
			kind = task->m_kind;
			period = task->m_period;
			data = task->m_data;
			semicolon = strchr(data, ';');
			if (semicolon) {
				*semicolon = '\0';
				objectId = atoi(data);
				objectId = MapResourceId(objectId);
				*semicolon = ';';
			}

			if (kind >= 0 && kind < 6) {
				index = FindStarIdxById(objectId);
				if (index != -1) {
					AttachTaskToObj(index, g_taskFns[kind], period, data);
				}
				else {
					CreateDetachedTask(&g_detachedTasks, g_taskFns[kind], period, data);
				}
			}
			else {
				Error(0x33, NULL);
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdPosition]) {
			SceneObject* obj;
			BwdPlaceRecord* position = (BwdPlaceRecord*) node;
			MechS32 x;
			MechS32 y;
			MechS32 z;
			Shape* shape;
			MechS32 id;

			id = position->m_id;
			x = position->m_x;
			y = position->m_y;
			z = position->m_z;
			id = MapResourceId(id);
			shape = FindClassById(id);
			if (shape) {
				obj = GetShapeObject(shape);
				if (obj) {
					SetObjPosition(obj, x, y, z);
					UpdateObj(obj);
				}
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdRotate]) {
			SceneObject* obj;
			MechS32 x;
			MechS32 y;
			MechS32 z;
			Shape* shape;
			BwdPlaceRecord* rotate = (BwdPlaceRecord*) node;
			MechS32 id;

			id = rotate->m_id;
			x = rotate->m_x;
			y = rotate->m_y;
			z = rotate->m_z;
			id = MapResourceId(id);
			shape = FindClassById(id);
			if (shape) {
				obj = GetShapeObject(shape);
				if (obj) {
					SetObjRotation(
						obj,
						(MechS32) (x * 65536.0 + 0.5),
						(MechS32) (y * 65536.0 + 0.5),
						(MechS32) (z * 65536.0 + 0.5),
						0
					);
					UpdateObj(obj);
				}
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdInclude]) {
			result &= ExecuteInclude((IncludeRecord*) node, BwdExecuteStream);
		}
		else if (type == g_bwdTypeCodes[c_bwdGroup]) {
			SetTeamFormations((struct FormationNames*) node);
		}
		else if (type == g_bwdTypeCodes[c_bwdGpSpec]) {
			Mech* mech;
			MechS32 isLocal;
			MechS32 leader;
			MechChar name[9];
			MechU32 events;
			MechS32 i;
			MechS32 index;
			IncludeRecord2* gpSpec = (IncludeRecord2*) node;
			MechChar config[9];
			MechS32 id;
			MechS32 mekId;
			MechU16 team;
			MechU16 target;
			MechS32 ai;

			index = 0;
			isLocal = FALSE;
			mekId = gpSpec->m_mekId;
			id = gpSpec->m_id;
			strncpy(name, gpSpec->m_name, 8);
			name[8] = '\0';
			strncpy(config, gpSpec->m_config, 8);
			config[8] = '\0';
			team = gpSpec->m_team;
			if (team >= 0x10) {
				team = 0;
			}

			leader = gpSpec->m_leader;
			ai = gpSpec->m_ai;
			if (ai == 0) {
				g_localStar = team;
				g_loadHudFile = 1;
			}

			events = WidenEventFlags(gpSpec->m_events);
			if (RunIncludedStream(gpSpec, BwdExecuteStream)) {
				if (g_lastPlayer) {
					index = g_lastPlayer->m_index;
					for (i = 0; i < 8; i++) {
						g_lastPlayer->m_aiParams[i] = gpSpec->m_aiParams[i];
					}

					g_lastPlayer->m_aiMode = ai;
					g_lastPlayer->m_flags = gpSpec->m_flags;
					strncpy(g_lastPlayer->m_name, gpSpec->m_playerName, 0x16);
					g_lastPlayer->m_name[0x15] = '\0';
					strncpy(g_lastPlayer->m_shortName, gpSpec->m_playerShortName, 0x16);
					g_lastPlayer->m_shortName[0x15] = '\0';
					if (eventList) {
						target = (MechU8) index | 0x200;
						PostEventToList(eventName, events, target);
					}

					g_lastPlayer->m_team = team;
					SetPlayerSlot(index, g_teams[team].m_memberCount);
					if (leader == 1) {
						SetTeamLeader(team, index);
					}

					g_teams[team].m_members[g_teams[team].m_memberCount] = index;
					g_teams[team].m_memberCount++;
					mech = g_lastPlayer->m_mech;
					if (ai == 0) {
						isLocal = TRUE;
					}

					if (mech && !LoadMechConfig(mech, name, mekId, config)) {
						Error(0x43, NULL);
						result = FALSE;
					}
				}
				else {
					Error(0x43, NULL);
				}
			}
			else {
				result = FALSE;
			}
		}
		else if (type == g_bwdTypeCodes[c_bwdMangleOff]) {
			SetMangleBase(0);
		}
		else if (type == g_bwdTypeCodes[c_bwdMangleOn]) {
			g_nextMangleBase += 0x10000;
			SetMangleBase(g_nextMangleBase);
		}
		else if (type == g_bwdTypeCodes[c_bwdAnim2d]) {
			BwdAnim2dSpec* spec;
			BwdAnim2dRecord* anim2d = (BwdAnim2dRecord*) node;

			spec = &anim2d->m_spec;
			LoadAnim2d(spec->m_flags, spec->m_frameTime, spec->m_type, &spec->m_resourceId);
		}
		else if (type == g_bwdTypeCodes[c_bwdRev]) {
		}
		else if (type == g_bwdTypeCodes[c_bwdDtbl]) {
		}
		else {
			Error(8, NULL);
		}
	}

	if (eventList) {
		FlushEventLists(eventList);
	}

	return result;
}

// Loads the mission world p_name (a resource number or a file name): resets the teams, the
// thing table and the shape offset, then executes its BWD stream. Returns whether it loaded.
// Stack-slot permutation of the locals.
// FUNCTION: MW2 0x1000d30f
MechS32 LoadWorld(MechChar* p_name)
{
	BwdStream* stream;
	BwdStreamKey* key;
	MechS32 result;
	MechS32 i;
	BwdStreamKey keyData;
	undefined buffer[0x20];

	result = FALSE;
	ResetTeams();
	for (i = 0; i < 0x96; i++) {
		g_thingRecordIndices[i] = -1;
	}

	g_thingRecordCount = 0;
	g_nextThingRecord = 0;
	SetShapeOffset(0, 0, 0);
	key = &keyData;
	if (isdigit(*p_name)) {
		key->m_id = atoi(p_name);
	}
	else {
		key->m_id = -1;
	}

	strncpy(key->m_name, p_name, 0xc);
	key->m_name[0xc] = '\0';
	SetMangleBase(0);
	stream = OpenBwdStream(key, (BwdStream*) buffer);
	if (stream && AllocGeoTables()) {
		result = BwdExecuteStream(stream);
		UnloadResource(stream);
		FreeMissionTables();
		FreeGeoTables();
		if (!AreStaticObjectsComplete()) {
			Error(0x4b, NULL);
		}
	}

	if (!result) {
		Error(0xb, "%s", key->m_name);
		return FALSE;
	}
	else {
		return TRUE;
	}
}

// Counts the players and game things of each side and gives each game thing the radius of its
// object's shape. In a network game, every player's team but the local one is on side 1.
// Stack-slot permutation: obj and i. Operand order: i == g_localStar loads
// g_localStar first in the original.
// FUNCTION: MW2 0x1000d4a6
void AfterWorldLoader(void)
{
	SceneObject* obj;
	MechS32 i;

	for (i = 0; i < g_playerCount; i++) {
		if (g_isNetworkGame) {
			if (i == g_localStar) {
				g_teams[i].m_side = 0;
			}
			else {
				g_teams[i].m_side = 1;
			}
		}

		switch (GetPlayerSide(i)) {
		case 0:
			g_careerRecord.m_sideCounts[0]++;
			break;
		case 2:
			g_careerRecord.m_sideCounts[1]++;
			break;
		case 1:
			g_careerRecord.m_sideCounts[2]++;
			break;
		}
	}

	for (i = 0; i < g_gameThingCount; i++) {
		switch (GetThingSide(i)) {
		case 0:
			g_careerRecord.m_sideCounts[3]++;
			break;
		case 2:
			g_careerRecord.m_sideCounts[4]++;
			break;
		case 1:
			g_careerRecord.m_sideCounts[5]++;
			break;
		}

		obj = GetStaticSceneObject(g_gameThings[i].m_staticObject);
		if (obj && obj->m_shape) {
			g_gameThings[i].m_radius = obj->m_shape->m_radius;
		}
	}
}
