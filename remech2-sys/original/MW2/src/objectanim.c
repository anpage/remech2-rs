/* In the original, GetViewVertex, ClipEdgeToNearPlane, ProjectVertex and GetFaceShade are C
   functions with __asm bodies, and QueueFace has __asm blocks. The portable C here replaces each
   whole function, and wraps where standard C overflows. */
#include "objectanim.h"

#include "ambientsound.h"
#include "callbacks.h"
#include "classtable.h"
#include "clock.h"
#include "config.h"
#include "depthsort.h"
#include "face.h"
#include "fixeddiv.h"
#include "fixedmul.h"
#include "fixedtrig.h"
#include "geocache.h"
#include "object.h"
#include "path.h"
#include "players.h"
#include "polydraw.h"
#include "poolsizes.h"
#include "portable.h"
#include "projectedvertex.h"
#include "queuedpolygon.h"
#include "ramp.h"
#include "recordstacks.h"
#include "reel.h"
#include "reelevent.h"
#include "resource.h"
#include "resourcename.h"
#include "shape.h"
#include "simmain.h"
#include "soundfx.h"
#include "staticmem.h"
#include "types.h"
#include "vertex.h"
#include "view.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// The state of ColorCycleTask's callback: the faces of a shape cycle through up to sixteen colors.
typedef struct ColorCycle {
	Shape** m_shape;      // 0x00 — the star's shape (GetStaticShapeSlot)
	Shape* m_model;       // 0x04
	int32_t m_faceCount;  // 0x08 — -1 until counted
	int32_t m_colorCount; // 0x0c
	int32_t m_colors[16]; // 0x10
} ColorCycle;

// The state of SpinTask's callback: an object turning at constant rates.
typedef struct ObjectSpin {
	Shape** m_shape;       // 0x00
	SceneObject* m_object; // 0x04
	int32_t m_rateX;       // 0x08 — 16.16 per period
	int32_t m_rateY;       // 0x0c
	int32_t m_rateZ;       // 0x10
	int32_t m_period;      // 0x14 — clock ticks
	int32_t m_lastClock;   // 0x18
} ObjectSpin;

// The state of OrbitTask's callback: an object circling while it turns.
typedef struct ObjectOrbit {
	Shape** m_shape;       // 0x00
	SceneObject* m_object; // 0x04
	int32_t m_angle;       // 0x08 — 16.16 degrees
	int32_t m_turnRate;    // 0x0c
	int32_t m_speed;       // 0x10
	int32_t m_enabled;     // 0x14
	int32_t m_lastClock;   // 0x18
} ObjectOrbit;

// The state of ReelMotionTask's callback: an object moving through an animation (Reel)
// in step with its player's animation state (m_animFlags to m_animRate).
typedef struct ReelMotion {
	Reel* m_reel;          // 0x00
	Reel* m_initial;       // 0x04
	SceneObject* m_object; // 0x08
	Player* m_player;      // 0x0c
	int32_t m_rate;        // 0x10 — clock ticks per frame
	int32_t m_timer;       // 0x14 — until the next frame
	int32_t m_amount;      // 0x18 — left to move in this frame
	int32_t m_lastClock;   // 0x1c
	int32_t m_enabled;     // 0x20
	int32_t m_frame;       // 0x24 — -1 before the first
	uint32_t m_flags;      // 0x28 — 1: drives the player's animation state
} ReelMotion;

// The state of PathTask's callback: a star's object following a path, eased by the ramps.
typedef struct PathFollower {
	Shape** m_shape;       // 0x00 — the star's shape (GetStaticShapeSlot)
	SceneObject* m_object; // 0x04
	Path* m_path;          // 0x08
	int32_t m_startClock;  // 0x0c
	int32_t m_duration;    // 0x10 — the points' times added up
	int32_t m_rotate;      // 0x14 — turns along the path
	int32_t m_mode;        // 0x18 — at the end: 0 "loop" runs on, 1 "repeat" restarts, 2 stops
	Ramp m_x;              // 0x1c
	Ramp m_y;              // 0x2c
	Ramp m_z;              // 0x3c
	WrappedRamp m_pitch;   // 0x4c
	WrappedRamp m_heading; // 0x60
	WrappedRamp m_roll;    // 0x74
} PathFollower;

// Why ReelMotionTask last failed: 1 no player, 2 disabled, 3 no object, 4-7 missing state.
int32_t g_reelMotionError = 0;

// The number of entries in g_paths.
int32_t g_pathCount = 0;

int32_t g_maxAnimNumber = 0;

// The base of the animation numbers of the file being loaded.
int32_t g_animBase = 0;

// The number of entries in g_animFiles.
int32_t g_animFileCount = 0;

// Set to shade from the origin rather than the light (GetFaceShade).
int32_t g_directionalLight;

// The outcodes of the polygon being built: any vertex's (or) and every vertex's (and).
uint8_t g_polygonOrCodes;

uint8_t g_polygonAndCodes;

// The projected vertices of the polygon being built, and their count.
ProjectedVertex* g_polygonPoints[20];

int32_t g_polygonPointCount;

// Where QueueFace copies the next polygon's vertex pointers in the draw buffer.
uint8_t* g_polygonPointCursor;

// QueueFace's counts: faces it took (5bc), faces past the back-face test (538), vertices it
// projected (5b4) and polygons it queued (5a8).

int32_t g_facesTried;

int32_t g_facesFrontFacing;

int32_t g_verticesTransformed;

int32_t g_polygonsQueued;

Path g_paths[0x40];

// The animations of the loaded animation files, by number.
Reel* g_reels[0x780];

AnimFile g_animFiles[60];

// A timed callback (TimedCallbackFn) moving a thing's object through an animation. Its data is
// "<thing or class id>;<rate>,<flags>,<animation>", made for the player being created
// (g_lastPlayer). Each frame moves or turns the object by the frame's amount, spread over
// the rate; the frame events jump to other frames by the player's animation state (m_motionState,
// the frame it reached, and m_nextMotionState, the one it wants).
int32_t ReelMotionTask(int32_t p_event, char* p_data, int32_t p_clock, int32_t p_period)
{
	int32_t next;
	int32_t elapsed;
	int32_t y;
	char* token;
	int32_t z;
	int32_t jump;
	ReelMotion* motion;
	int32_t thing;
	int32_t back;
	int32_t enabled;
	int32_t found;
	int32_t amount;
	int32_t turnX;
	int32_t step;
	int32_t target;
	int32_t turnY;
	int32_t value;
	int32_t number;
	int32_t turnZ;
	int32_t frame;
	Shape* shape;
	int32_t rate;
	int32_t id;
	void** slot;
	int32_t x;
	int32_t match;
	int32_t i;

	rate = 0;
	enabled = 0;
	number = 0;
	jump = FALSE;
	found = FALSE;
	thing = -1;
	switch (p_event) {
	case 0:
		motion = StaticPoolAlloc(sizeof(ReelMotion), g_staticPoolTags[4]);
		if (!motion) {
			return 0;
		}

		if (!g_lastPlayer) {
			g_reelMotionError = 1;
			MechHeapFree(g_primaryHeap, motion);
			return 0;
		}

		slot = GetCallbackData(GetCurrentCallback());
		*slot = motion;
		motion->m_object = NULL;
		motion->m_reel = NULL;
		motion->m_enabled = 0;
		motion->m_rate = 0;
		motion->m_flags = 0;
		motion->m_player = g_lastPlayer;
		token = strchr(p_data, ';');
		if (token) {
			*token = '\0';
			token++;
			sscanf(token, "%d,%d,%d", &rate, &enabled, &number);
			number += g_animBase;
			if (number > g_maxAnimNumber) {
				g_maxAnimNumber = number;
			}

			if (enabled) {
				motion->m_enabled = 1;
				if (enabled > 0x7f) {
					motion->m_flags |= 1;
				}
			}
			else {
				motion->m_enabled = 0;
				g_reelMotionError = 2;
			}

			motion->m_rate = rate;
			motion->m_reel = g_reels[number];
			motion->m_initial = g_reels[number];
			motion->m_frame = -1;
			motion->m_timer = 0;
			motion->m_amount = 0;
			motion->m_lastClock = p_clock;
			motion->m_player->m_animRate = rate;
			motion->m_player->m_motionState = -1;
		}

		id = atoi(p_data);
		id = MapResourceId(id);
		thing = FindThingIdxById(id);
		if (thing != -1) {
			motion->m_object = GetClassObject(thing);
		}
		else {
			shape = FindClassById(id);
			if (shape) {
				motion->m_object = GetShapeObject(shape);
			}
		}

		if (!motion->m_object) {
			g_reelMotionError = 3;
		}
		break;
	case 1:
		slot = GetCallbackData(GetCurrentCallback());
		motion = *slot;
		if (!motion) {
			return 0;
		}

		if (!motion->m_object) {
			return 0;
		}

		if (!motion->m_player) {
			g_reelMotionError = 5;
			return 0;
		}

		if (!motion->m_enabled) {
			g_reelMotionError = 4;
			return 0;
		}

		elapsed = p_clock - motion->m_lastClock;
		motion->m_lastClock = p_clock;
		if (motion->m_player->m_animFlags & 0x10) {
			motion->m_reel = motion->m_initial;
			motion->m_frame = -1;
			motion->m_timer = 0;
			motion->m_amount = 0;
			motion->m_lastClock = p_clock;
			if (motion->m_flags & 1) {
				motion->m_player->m_animRate = motion->m_rate;
				motion->m_player->m_animFlags &= ~0x11;
				motion->m_player->m_motionState = -1;
			}
		}
		else if (motion->m_player->m_animFlags & 1) {
			if (!motion->m_rate) {
				g_reelMotionError = 6;
				return 0;
			}

			if (!motion->m_player->m_animRate) {
				g_reelMotionError = 7;
				return 0;
			}

			if (motion->m_timer <= 0) {
				amount = motion->m_amount;
			}
			else {
				amount = motion->m_amount * elapsed / motion->m_timer;
			}

			if (motion->m_amount < 0) {
				if (motion->m_amount > amount) {
					amount = motion->m_amount;
				}
			}
			else if (motion->m_amount < amount) {
				amount = motion->m_amount;
			}

			if (motion->m_reel->m_kind >= 3) {
				turnX = turnY = turnZ = 0;
				switch (motion->m_reel->m_kind) {
				case 3:
					turnX = amount;
					break;
				case 4:
					turnY = amount;
					break;
				case 5:
					turnZ = amount;
					break;
				}

				RotateObj(motion->m_object, turnX, turnY, turnZ, 0);
			}
			else {
				x = y = z = 0;
				switch (motion->m_reel->m_kind) {
				case 0:
					x = amount;
					break;
				case 1:
					y = amount;
					break;
				case 2:
					z = amount;
					break;
				}

				MoveObj(motion->m_object, x, y, z);
			}

			UpdateObj(motion->m_object);
			motion->m_amount -= amount;
			motion->m_timer -= elapsed;
			if (motion->m_frame == -1) {
				motion->m_frame = 0;
				motion->m_timer = 0;
				motion->m_player->m_motionState = 0;
			}

			if (motion->m_timer <= 0) {
				frame = motion->m_frame;
				motion->m_frame++;
				motion->m_timer += motion->m_player->m_animRate;
				if (-motion->m_player->m_animRate > motion->m_timer) {
					motion->m_timer = -motion->m_player->m_animRate;
				}

				if (motion->m_reel->m_events[frame].m_flags & 0x40) {
					target = motion->m_reel->m_events[frame].m_values[1];
					if (motion->m_player->m_nextMotionState == target) {
						jump = TRUE;
					}
					else {
						jump = FALSE;
					}
				}

				if (motion->m_reel->m_events[frame].m_flags & 0x80) {
					target = motion->m_reel->m_events[frame].m_values[1];
					if (motion->m_player->m_nextMotionState != -1 &&
						motion->m_player->m_motionState != motion->m_player->m_nextMotionState) {
						jump |= TRUE;
					}
				}

				if (motion->m_reel->m_events[frame].m_flags & 0x100) {
					target = motion->m_reel->m_events[frame].m_values[2];
					if (motion->m_player->m_nextMotionState == -1) {
						jump |= TRUE;
					}
				}

				if (jump) {
					jump = FALSE;
					if (motion->m_player->m_motionState < target) {
						step = 1;
					}
					else {
						step = -1;
					}

					found = FALSE;
					next = frame;
					while (!found) {
						next += step;
						if (next >= motion->m_reel->m_frameCount) {
							next = -1;
						}

						if (next == -1) {
							found = TRUE;
						}
						else if (motion->m_reel->m_events[next].m_flags & 1) {
							value = motion->m_reel->m_events[next].m_values[0];
							if (value == target) {
								found = TRUE;
							}
						}
					}

					if (next != -1) {
						found = FALSE;
						while (!found) {
							if (next >= motion->m_reel->m_frameCount) {
								next = -1;
								found = TRUE;
							}
							else {
								if (motion->m_reel->m_events[next].m_flags & 0x200) {
									value = motion->m_reel->m_events[next].m_values[3];
									if (motion->m_player->m_motionState == value) {
										found = TRUE;
									}
								}

								next++;
							}
						}
					}

					if (next != -1) {
						motion->m_frame = next;
						jump = TRUE;
					}
				}

				if (!jump && (motion->m_reel->m_events[frame].m_flags & 0xc)) {
					match = TRUE;
					if (motion->m_reel->m_events[frame].m_flags & 4) {
						match = FALSE;
						for (i = 0; i < 4; i++) {
							if (motion->m_reel->m_events[frame].m_values[i] == -1) {
								break;
							}

							if (motion->m_reel->m_events[frame].m_values[i] ==
								(int8_t) motion->m_player->m_nextMotionState) {
								match = TRUE;
							}
							else {
								match = FALSE;
							}

							if (match) {
								break;
							}
						}
					}

					if (match) {
						back = frame - 1;
						while (!(motion->m_reel->m_events[back].m_flags & 2) && back >= 0) {
							back--;
						}

						motion->m_frame = back;
					}
				}

				if ((!jump && (motion->m_reel->m_events[frame].m_flags & 0x400)) ||
					motion->m_frame >= motion->m_reel->m_frameCount) {
					motion->m_timer = 0;
					motion->m_frame = -1;
				}

				if (motion->m_frame != -1) {
					motion->m_amount += motion->m_reel->m_amounts[motion->m_frame];
				}

				if (motion->m_flags & 1) {
					if (jump) {
						motion->m_player->m_motionState = target;
					}

					motion->m_player->m_animFlags &= ~2;
					motion->m_player->m_animFlags &= ~8;
					if (motion->m_frame != -1) {
						motion->m_player->m_animRate =
							ScaleBySpeedLevel(motion->m_player->m_speedLevel, motion->m_rate);
						if (motion->m_reel->m_events[motion->m_frame].m_flags & 0x10) {
							motion->m_player->m_animFlags |= 2;
						}

						if (motion->m_reel->m_events[motion->m_frame].m_flags & 0x800) {
							motion->m_player->m_animFlags |= 8;
						}
					}
					else if (motion->m_player->m_nextMotionState == -1) {
						motion->m_player->m_animFlags &= ~1;
						motion->m_player->m_motionState = -1;
					}
				}
			}
		}
		else if (motion->m_flags & 1) {
			if (motion->m_player->m_nextMotionState != -1 || motion->m_player->m_motionState != -1) {
				motion->m_player->m_animFlags |= 1;
				motion->m_player->m_animRate = ScaleBySpeedLevel(motion->m_player->m_speedLevel, motion->m_rate);
			}
		}
		break;
	default:
		break;
	}

	return 1;
}

// Scales p_value by mode p_mode: 1 by 1.5, 3 by 0.75, any other mode leaves it.
int32_t ScaleBySpeedLevel(int32_t p_mode, int32_t p_value)
{
	int32_t result;

	switch (p_mode) {
	case 1:
		result = (p_value >> 1) + p_value;
		break;
	case 2:
		result = p_value;
		break;
	case 3:
		result = p_value - (p_value >> 2);
		break;
	default:
		result = p_value;
		break;
	}

	return result;
}

// Loads the animation file p_ref unless it has been, and sets the base of its animation numbers
// (g_animBase). Returns -1 if it was already loaded.
int32_t LoadAnimFile(ResourceRef* p_ref)
{
	int32_t result;
	int32_t i;
	int32_t id;

	result = 0;
	id = p_ref->m_id;
	for (i = 0; i < g_animFileCount && g_animFiles[i].m_id != id; i++) {
	}

	if (i < g_animFileCount) {
		g_animBase = g_animFiles[i].m_base;
		result = -1;
	}
	else if (g_animFileCount < 60) {
		if (g_maxAnimNumber > 0) {
			g_animBase = g_maxAnimNumber + 1;
		}

		result = LoadReels(p_ref);
		g_animFiles[g_animFileCount].m_base = g_animBase;
		g_animFiles[g_animFileCount].m_id = id;
		g_animFileCount++;
	}

	return result;
}

int32_t GetAnimBase(void)
{
	return g_animBase;
}

int32_t GetReelMotionSize(void)
{
	return sizeof(ReelMotion);
}

// A timed callback (TimedCallbackFn) cycling the face colors of a star's shape. Its data is
// "<star id>;<color>,<color>,...": up to sixteen colors, each a palette row.
int32_t ColorCycleTask(int32_t p_event, char* p_data, int32_t p_clock, int32_t p_period)
{
	char* token;
	ColorCycle* cycle;
	int32_t vertexCount;
	int32_t offset;
	int32_t i;
	int32_t star;
	int32_t id;
	void** slot;

	if (!p_period) {
		p_period = 1;
	}

	switch (p_event) {
	case 0:
		cycle = MechHeapAlloc(g_primaryHeap, sizeof(ColorCycle));
		if (!cycle) {
			return 0;
		}

		slot = GetCallbackData(GetCurrentCallback());
		*slot = cycle;
		cycle->m_colorCount = 0;
		cycle->m_model = NULL;
		cycle->m_faceCount = -1;
		token = strchr(p_data, ';');
		if (token) {
			*token = '\0';
			token++;
			for (token = strtok(token, ","); token; token = strtok(NULL, ",")) {
				if (cycle->m_colorCount < 16) {
					cycle->m_colors[cycle->m_colorCount] = strtoul(token, NULL, 0) << 4;
					cycle->m_colorCount++;
				}
				else {
					break;
				}
			}
		}

		id = atoi(p_data);
		id = MapResourceId(id);
		star = FindStarIdxById(id);
		if (star != -1) {
			cycle->m_shape = GetStaticShapeSlot(star);
		}
		else {
			return 0;
		}
		break;
	case 1:
		slot = GetCallbackData(GetCurrentCallback());
		cycle = *slot;
		if (!cycle) {
			return 0;
		}
		else if (!cycle->m_shape) {
			return 0;
		}

		if (!*cycle->m_shape) {
			return 0;
		}

		cycle->m_model = *cycle->m_shape;
		if (cycle->m_faceCount == -1) {
			GetModelCounts(cycle->m_model, &vertexCount, &cycle->m_faceCount);
		}

		offset = p_clock / p_period % cycle->m_colorCount;
		for (i = 0; i < cycle->m_faceCount; i++) {
			SetFaceColor(cycle->m_model, i, cycle->m_colors[(offset + i) % cycle->m_colorCount]);
		}
		break;
	default:
		break;
	}

	return 1;
}

// A timed callback (TimedCallbackFn) turning a star's object. Its data is
// "<star id>;<x>,<y>,<z>,<period>": the turns in degrees per period of seconds.
int32_t SpinTask(int32_t p_event, char* p_data, int32_t p_clock, int32_t p_period)
{
	int32_t dz;
	char* token;
	ObjectSpin* spin;
	float x;
	float y;
	float z;
	float period;
	int32_t star;
	int32_t t;
	int32_t id;
	int32_t dx;
	void** slot;
	int32_t dy;

	switch (p_event) {
	case 0:
		spin = MechHeapAlloc(g_primaryHeap, sizeof(ObjectSpin));
		if (!spin) {
			return 0;
		}

		slot = GetCallbackData(GetCurrentCallback());
		*slot = spin;
		spin->m_rateX = spin->m_rateY = spin->m_rateZ = 0;
		spin->m_object = NULL;
		token = strchr(p_data, ';');
		if (token) {
			*token = '\0';
			token++;
			sscanf(token, "%f,%f,%f,%f", &x, &y, &z, &period);
			spin->m_rateX = x * 65536.0 + 0.5;
			spin->m_rateY = y * 65536.0 + 0.5;
			spin->m_rateZ = z * 65536.0 + 0.5;
			spin->m_period = period * 181.0f;
			if (!spin->m_period) {
				spin->m_period = 181;
			}

			spin->m_lastClock = p_clock;
		}

		id = atoi(p_data);
		id = MapResourceId(id);
		star = FindStarIdxById(id);
		if (star != -1) {
			spin->m_shape = GetStaticShapeSlot(star);
		}
		else {
			return 0;
		}
		break;
	case 1:
		slot = GetCallbackData(GetCurrentCallback());
		spin = *slot;
		if (!spin) {
			return 0;
		}
		else if (!spin->m_shape) {
			return 0;
		}

		if (!*spin->m_shape) {
			return 0;
		}

		spin->m_object = GetShapeObject(*spin->m_shape);
		t = ((p_clock - spin->m_lastClock) << 16) / spin->m_period;
		dx = FixedMul16(spin->m_rateX, t);
		dy = FixedMul16(spin->m_rateY, t);
		dz = FixedMul16(spin->m_rateZ, t);
		spin->m_lastClock = p_clock;
		RotateObj(spin->m_object, dx, dy, dz, 0);
		UpdateObj(spin->m_object);
		break;
	default:
		break;
	}

	return 1;
}

// A timed callback (TimedCallbackFn) moving a star's object around a circle. Its data is
// "<star id>;<radius>,<period>,<enabled>,<unused>".
int32_t OrbitTask(int32_t p_event, char* p_data, int32_t p_clock, int32_t p_period)
{
	char* token;
	int32_t z;
	int32_t cosine;
	ObjectOrbit* orbit;
	int32_t t;
	int32_t step;
	int32_t star;
	int32_t sine;
	int32_t enabled;
	float radius;
	int32_t unused;
	float period;
	int32_t id;
	void** slot;
	int32_t x;

	switch (p_event) {
	case 0:
		orbit = MechHeapAlloc(g_primaryHeap, sizeof(ObjectOrbit));
		if (!orbit) {
			return 0;
		}

		slot = GetCallbackData(GetCurrentCallback());
		*slot = orbit;
		orbit->m_enabled = 0;
		orbit->m_object = NULL;
		token = strchr(p_data, ';');
		if (token) {
			*token = '\0';
			token++;
			sscanf(token, "%f,%f,%d,%d", &radius, &period, &enabled, &unused);
			orbit->m_turnRate = 360.0 / period * 65536.0 + 0.5;
			orbit->m_speed = radius * 6.283185307179586 / period * 65536.0 + 0.5;
			orbit->m_enabled = enabled;
			orbit->m_angle = 0;
		}

		orbit->m_lastClock = p_clock;
		id = atoi(p_data);
		id = MapResourceId(id);
		star = FindStarIdxById(id);
		if (star != -1) {
			orbit->m_shape = GetStaticShapeSlot(star);
		}
		else {
			return 0;
		}
		break;
	case 1:
		slot = GetCallbackData(GetCurrentCallback());
		orbit = *slot;
		if (!orbit) {
			return 0;
		}
		else if (!orbit->m_shape || !orbit->m_enabled) {
			return 0;
		}

		if (!*orbit->m_shape) {
			return 0;
		}

		orbit->m_object = GetShapeObject(*orbit->m_shape);
		cosine = FixedCos(orbit->m_angle);
		sine = FixedSin(orbit->m_angle);
		t = FixedDiv16(p_clock - orbit->m_lastClock, 181);
		orbit->m_lastClock = p_clock;
		step = FixedMul16(orbit->m_speed, t) >> 16;
		x = FixedMul16(cosine, step) >> 13;
		z = FixedMul16(-sine, step) >> 13;
		MoveObj(orbit->m_object, x, 0, z);
		UpdateObj(orbit->m_object);
		step = FixedMul16(orbit->m_turnRate, t);
		RotateObj(orbit->m_object, 0, step, 0, 0);
		UpdateObj(orbit->m_object);
		orbit->m_angle += step;
		orbit->m_angle %= 0x1680000;
		break;
	default:
		break;
	}

	return 1;
}

// A timed callback (TimedCallbackFn) looping a sound on a star's object. Its data is
// "<star id>;<range>,<sound name>,<enabled>".
int32_t AmbientSoundTask(int32_t p_event, char* p_data, int32_t p_clock, int32_t p_period)
{
	char* token;
	AmbientSound* sound;
	int32_t star;
	int32_t id;
	void** slot;
	char name[100];

	switch (p_event) {
	case 0:
		sound = MechHeapAlloc(g_primaryHeap, sizeof(AmbientSound));
		if (!sound) {
			return 0;
		}

		slot = GetCallbackData(GetCurrentCallback());
		*slot = sound;
		sound->m_slot = -1;
		sound->m_data = NULL;
		sound->m_skip = 1;
		token = strchr(p_data, ';');
		if (token) {
			*token = '\0';
			token++;
			sscanf(token, "%d,%[^,],%d", &sound->m_range, name, &sound->m_enabled);
			sound->m_range *= 100;
		}

		sound->m_id = FindResourceIdByName(0xb, name);
		id = atoi(p_data);
		id = MapResourceId(id);
		star = FindStarIdxById(id);
		if (star != -1) {
			sound->m_shape = GetStaticShapeSlot(star);
		}
		else {
			return 0;
		}
		break;
	case 1:
		slot = GetCallbackData(GetCurrentCallback());
		sound = *slot;
		if (!sound) {
			return 0;
		}
		else if (!sound->m_shape || !sound->m_enabled) {
			StopAmbientSound(sound);
			return 0;
		}

		if (!*sound->m_shape) {
			StopAmbientSound(sound);
			return 0;
		}

		sound->m_obj = GetShapeObject(*sound->m_shape);
		UpdateAmbientSound(sound);
		break;
	case 2:
		slot = GetCallbackData(GetCurrentCallback());
		sound = *slot;
		if (!sound) {
			return 1;
		}

		StopAmbientSound(sound);
		break;
	}

	return 1;
}

// A timed callback (TimedCallbackFn) moving a star's object along a path. Its data is
// "<star id>;<mode>,<rotate>,<path name>": mode "loop", "repeat" or else stop at the end, and
// "rotate" to turn the object along the path. Event -1 restarts it.
int32_t PathTask(int32_t p_event, char* p_data, int32_t p_clock, int32_t p_period)
{
	char rotate[64];
	char* token;
	PathFollower* follower;
	int32_t total;
	char name[64];
	int32_t i;
	char mode[64];
	int32_t star;
	int32_t id;
	void** slot;
	PathPoint* first;
	PathPoint* origin;
	int32_t elapsed;
	Path* path;
	int32_t start;
	int32_t end;
	PathPoint* point;
	int32_t t;
	int32_t delta;
	PathPoint* next;
	int32_t angle;

	switch (p_event) {
	case -1:
		slot = GetCallbackData(GetCurrentCallback());
		follower = *slot;
		if (!follower) {
			return 0;
		}

		follower->m_startClock = p_clock;
		if (follower->m_path) {
			first = follower->m_path->m_points;
			StartRamp(&follower->m_x, first->m_x, first->m_x, 0.3);
			StartRamp(&follower->m_y, first->m_y, first->m_y, 0.3);
			StartRamp(&follower->m_z, first->m_z, first->m_z, 0.3);
			StartWrappedRamp(&follower->m_pitch, 0, 0, 0.3, 0x1680000);
			StartWrappedRamp(&follower->m_heading, 0, 0, 0.3, 0x1680000);
			StartWrappedRamp(&follower->m_roll, 0, 0, 0.3, 0x1680000);
			total = 0;
			for (i = 0; i < follower->m_path->m_count; i++) {
				total += follower->m_path->m_points[i].m_duration;
			}

			follower->m_duration = total;
			id = atoi(p_data);
			id = MapResourceId(id);
			id = atoi(p_data);
			id = MapResourceId(id);
			star = FindStarIdxById(id);
			if (star != -1) {
				follower->m_shape = GetStaticShapeSlot(star);
			}
			else {
				return 0;
			}
		}
		break;
	case 0:
		follower = MechHeapAlloc(g_primaryHeap, sizeof(PathFollower));
		if (!follower) {
			return 0;
		}

		slot = GetCallbackData(GetCurrentCallback());
		*slot = follower;
		follower->m_object = NULL;
		follower->m_path = NULL;
		follower->m_startClock = p_clock;
		token = strchr(p_data, ';');
		if (token) {
			*token = '\0';
			token++;
			sscanf(token, "%[^,],%[^,],%[^,]", mode, rotate, name);
			if (strcasecmp(mode, "loop") == 0) {
				follower->m_mode = 0;
			}
			else if (strcasecmp(mode, "repeat") == 0) {
				follower->m_mode = 1;
			}
			else {
				follower->m_mode = 2;
			}

			if (strcasecmp(rotate, "rotate") == 0) {
				follower->m_rotate = 1;
			}
			else {
				follower->m_rotate = 0;
			}

			for (i = 0; i < g_pathCount; i++) {
				if (strcasecmp(name, g_paths[i].m_name) == 0) {
					follower->m_path = &g_paths[i];
					break;
				}
			}
		}

		if (follower->m_path) {
			origin = follower->m_path->m_points;
			StartRamp(&follower->m_x, origin->m_x, origin->m_x, 0.3);
			StartRamp(&follower->m_y, origin->m_y, origin->m_y, 0.3);
			StartRamp(&follower->m_z, origin->m_z, origin->m_z, 0.3);
			StartWrappedRamp(&follower->m_pitch, 0, 0, 0.3, 0x1680000);
			StartWrappedRamp(&follower->m_heading, 0, 0, 0.3, 0x1680000);
			StartWrappedRamp(&follower->m_roll, 0, 0, 0.3, 0x1680000);
			total = 0;
			for (i = 0; i < follower->m_path->m_count; i++) {
				total += follower->m_path->m_points[i].m_duration;
			}

			follower->m_duration = total;
			id = atoi(p_data);
			id = MapResourceId(id);
			id = atoi(p_data);
			id = MapResourceId(id);
			star = FindStarIdxById(id);
			if (star != -1) {
				follower->m_shape = GetStaticShapeSlot(star);
			}
			else {
				return 0;
			}
			break;
		}
		break;
	case 1:
		slot = GetCallbackData(GetCurrentCallback());
		follower = *slot;
		if (!follower) {
			return 0;
		}
		else if (!follower->m_shape || !follower->m_path) {
			return 0;
		}

		if (!*follower->m_shape) {
			return 0;
		}

		follower->m_object = GetShapeObject(*follower->m_shape);
		if (!follower->m_object) {
			return 0;
		}

		path = follower->m_path;
		if (path->m_count <= 1) {
			return 0;
		}

		elapsed = p_clock - follower->m_startClock;
		end = 0;
		start = 0;
		for (i = 0; i < path->m_count; i++) {
			start = end;
			end += path->m_points[i].m_duration;
			if (end >= elapsed) {
				break;
			}
		}

		if (i >= path->m_count - 1) {
			if (follower->m_mode == 2) {
				return 0;
			}
			else if (follower->m_mode == 1) {
				i = 0;
				elapsed = 0;
				follower->m_startClock = p_clock;
				start = 0;
				end = path->m_points[0].m_duration;
				StartRamp(&follower->m_x, path->m_points[0].m_x, path->m_points[0].m_x, 0.3);
				StartRamp(&follower->m_y, path->m_points[0].m_y, path->m_points[0].m_y, 0.3);
				StartRamp(&follower->m_z, path->m_points[0].m_z, path->m_points[0].m_z, 0.3);
				StartWrappedRamp(&follower->m_pitch, 0, 0, 0.3, 0x1680000);
				StartWrappedRamp(&follower->m_heading, 0, 0, 0.3, 0x1680000);
				StartWrappedRamp(&follower->m_roll, 0, 0, 0.3, 0x1680000);
			}
			else if (path->m_count == i) {
				i = 0;
				elapsed -= follower->m_duration;
				follower->m_startClock = p_clock - elapsed;
				start = 0;
				end = path->m_points[0].m_duration;
			}
		}

		point = &path->m_points[i];
		if (path->m_count - 1 == i) {
			next = path->m_points;
		}
		else {
			next = &path->m_points[i + 1];
		}

		t = FixedDiv16(elapsed - start, point->m_duration);
		follower->m_x.m_target = point->m_x + FixedMul16(t, next->m_x - point->m_x);
		follower->m_y.m_target = point->m_y + FixedMul16(t, next->m_y - point->m_y);
		follower->m_z.m_target = point->m_z + FixedMul16(t, next->m_z - point->m_z);
		SetObjPosition(
			follower->m_object,
			UpdateRamp(&follower->m_x),
			UpdateRamp(&follower->m_y),
			UpdateRamp(&follower->m_z)
		);
		angle = point->m_heading;
		if (follower->m_rotate) {
			angle += FixedAtan2(next->m_x - point->m_x, next->m_z - point->m_z);
		}

		delta = angle - follower->m_heading.m_value;
		while (delta > 0xb40000) {
			delta -= 0x1680000;
		}

		while (delta < -0xb40000) {
			delta += 0x1680000;
		}

		follower->m_heading.m_target = angle;
		follower->m_heading.m_value = angle - delta;
		angle = point->m_pitch;
		if (follower->m_rotate) {
			angle -= FixedAsin((next->m_y - point->m_y) << 13);
		}

		delta = angle - follower->m_pitch.m_value;
		while (delta > 0xb40000) {
			delta -= 0x1680000;
		}

		while (delta < -0xb40000) {
			delta += 0x1680000;
		}

		follower->m_pitch.m_target = angle;
		follower->m_pitch.m_value = angle - delta;
		angle = point->m_roll;
		delta = angle - follower->m_roll.m_value;
		while (delta > 0xb40000) {
			delta -= 0x1680000;
		}

		while (delta < -0xb40000) {
			delta += 0x1680000;
		}

		follower->m_roll.m_target = angle;
		follower->m_roll.m_value = angle - delta;
		SetObjRotation(
			follower->m_object,
			UpdateWrappedRamp(&follower->m_pitch),
			UpdateWrappedRamp(&follower->m_heading),
			UpdateWrappedRamp(&follower->m_roll),
			0
		);
		UpdateObj(follower->m_object);
		break;
	default:
		break;
	}

	return 1;
}

// The 64-bit product of two 32-bit values, as imul leaves it in edx:eax. Sums of products wrap,
// like the add/adc chains.
static MechU64 Product(int32_t p_a, int32_t p_b)
{
	return (MechU64) ((MechS64) p_a * p_b);
}

// sub: wraps.
static int32_t Difference(int32_t p_a, int32_t p_b)
{
	return PortableS32((uint32_t) p_a - (uint32_t) p_b);
}

// add: wraps.
static int32_t Sum(int32_t p_a, int32_t p_b)
{
	return PortableS32((uint32_t) p_a + (uint32_t) p_b);
}

// The view transform's row (p_a, p_b, p_c) times the vertex's offset from the eyepoint
// (g_viewEyeX, g_viewEyeY, g_viewEyeZ), shifted right by 27 and rounded.
static int32_t ViewRow(int32_t p_a, int32_t p_b, int32_t p_c, Vertex* p_vertex)
{
	MechU64 sum = Product(p_a, Difference(p_vertex->m_worldX, g_viewEyeX)) +
				  Product(p_b, Difference(p_vertex->m_worldY, g_viewEyeY)) +
				  Product(p_c, Difference(p_vertex->m_worldZ, g_viewEyeZ));

	return PortableS32(PortableShrdRound(sum, 27));
}

// A screen offset: p_value shifted left by p_shift (modulo 32, like the shld's count), divided by
// the depth, and rounded to a quarter.
static int32_t ProjectAxis(int32_t p_value, int32_t p_shift, int32_t p_depth)
{
	MechS64 scaled = (MechS64) p_value * ((MechS64) 1 << ((uint32_t) p_shift & 31));

	return PortableSar32(Sum(PortableIdiv(scaled, p_depth), 2), 2);
}

// The edge value at the near plane: p_from plus the share p_toPlane / p_span of the way to p_to.
static int32_t Interpolate(int32_t p_from, int32_t p_to, int32_t p_toPlane, int32_t p_span)
{
	return Sum(PortableIdiv((MechS64) Difference(p_from, p_to) * p_toPlane, p_span), p_to);
}

// Returns the vertex's projected copy (m_projection), transforming it into view space
// (g_viewProjX0's rows, from the eyepoint g_viewEyeY) the first time. The transform is an
// __asm block.
ProjectedVertex* GetViewVertex(Vertex* p_vertex)
{
	ProjectedVertex* result = p_vertex->m_projection;
	int32_t x;
	int32_t y;

	if (result) {
		return result;
	}

	result = AllocProjectedVertex();
	x = ViewRow(g_viewProjX0, g_viewProjX1, g_viewProjX2, p_vertex);
	y = ViewRow(g_viewProjY0, g_viewProjY1, g_viewProjY2, p_vertex);
	p_vertex->m_projection = result;
	result->m_x = x;
	result->m_y = y;
	result->m_z = PortableS32(p_vertex->m_depth);
	result->m_u = PortableS32(p_vertex->m_u << 16);
	result->m_v = PortableS32(p_vertex->m_v << 16);
	return result;
}

// Returns a new projected vertex where the edge from p_a to p_b crosses the near plane
// (g_viewNear), its position and texture coordinates interpolated. The body is an __asm
// block.
ProjectedVertex* ClipEdgeToNearPlane(Vertex* p_a, Vertex* p_b)
{
	ProjectedVertex* a;
	ProjectedVertex* b;
	ProjectedVertex* result;
	int32_t x0;
	int32_t y0;
	int32_t z0;
	int32_t u0;
	int32_t v0;
	int32_t x1;
	int32_t y1;
	int32_t z1;
	int32_t u1;
	int32_t v1;
	int32_t toPlane;
	int32_t span;

	a = p_a->m_projection;
	if (!a) {
		a = GetViewVertex(p_a);
	}

	b = p_b->m_projection;
	if (!b) {
		b = GetViewVertex(p_b);
	}

	result = AllocProjectedVertex();
	x0 = a->m_x;
	y0 = a->m_y;
	u0 = a->m_u;
	v0 = a->m_v;
	z0 = a->m_z;
	x1 = b->m_x;
	y1 = b->m_y;
	u1 = b->m_u;
	v1 = b->m_v;
	z1 = b->m_z;

	/* From the nearer end; the depths' difference wraps. */
	if (z1 <= z0) {
		span = Difference(z0, z1);
		if (span) {
			toPlane = Difference(g_viewNear, z1);
			x0 = Interpolate(x0, x1, toPlane, span);
			y0 = Interpolate(y0, y1, toPlane, span);
			u0 = Interpolate(u0, u1, toPlane, span);
			v0 = Interpolate(v0, v1, toPlane, span);
		}
	}
	else {
		span = Difference(z1, z0);
		if (span) {
			toPlane = Difference(g_viewNear, z0);
			x0 = Interpolate(x1, x0, toPlane, span);
			y0 = Interpolate(y1, y0, toPlane, span);
			u0 = Interpolate(u1, u0, toPlane, span);
			v0 = Interpolate(v1, v0, toPlane, span);
		}
	}

	result->m_x = x0;
	result->m_y = y0;
	result->m_z = g_viewNear;
	result->m_u = u0;
	result->m_v = v0;
	return result;
}

// Projects a view-space vertex onto the screen once per frame (m_projected), with its clip
// outcodes (m_outcode: 1 left, 2 right, 4 top, 8 bottom), accumulates the outcodes of the
// polygon being built and adds the vertex to its list (up to 20). The body is an __asm block.
ProjectedVertex* ProjectVertex(ProjectedVertex* p_vertex)
{
	int32_t screen;
	uint8_t outcode;

	if (!p_vertex->m_projected) {
		screen = Sum(ProjectAxis(p_vertex->m_x, g_viewShiftX, p_vertex->m_z), g_viewCenterX);
		p_vertex->m_screenX = screen;
		outcode = 0;
		if (screen > g_viewRight) {
			outcode |= 2;
		}
		if (screen < g_viewLeft) {
			outcode |= 1;
		}

		screen = Difference(g_viewCenterY, ProjectAxis(p_vertex->m_y, g_viewShiftY, p_vertex->m_z));
		p_vertex->m_screenY = screen;
		if (screen > g_viewBottom) {
			outcode |= 8;
		}
		if (screen < g_viewTop) {
			outcode |= 4;
		}

		p_vertex->m_outcode = outcode;
		p_vertex->m_projected = 1;
	}

	g_polygonOrCodes |= p_vertex->m_outcode;
	g_polygonAndCodes &= p_vertex->m_outcode;
	if (g_polygonPointCount >= 20) {
		g_queueHasRoom = 0;
	}
	else {
		g_polygonPoints[g_polygonPointCount++] = p_vertex;
	}

	return p_vertex;
}

// Returns the shade (0x7f: full) of p_face from the angle between its normal and the direction
// from its first vertex to the light (g_viewLightZ, or the origin with g_directionalLight). The
// shading is an __asm block.
int32_t GetFaceShade(Face* p_face, Vertex* p_vertices)
{
	Vertex* vertex = &p_vertices[((uint8_t*) p_face)[p_face->m_indexOffset]];
	int32_t x = vertex->m_worldX;
	int32_t y = vertex->m_worldY;
	int32_t z = vertex->m_worldZ;
	uint32_t magnitudeX;
	uint32_t magnitudeY;
	uint32_t magnitudeZ;
	uint32_t bits;
	uint32_t squares;
	MechU64 dot;
	int32_t scale;

	if (g_directionalLight) {
		x = y = z = 0;
	}

	/* The direction to the light, with each component's magnitude. A component is negated when
	   the light's coordinate is the smaller one, which leaves a difference that wrapped as it is. */
	dot = Product(Difference(g_viewLightX, x), p_face->m_normal[0]) +
		  Product(Difference(g_viewLightY, y), p_face->m_normal[1]);
	magnitudeX = (uint32_t) Difference(g_viewLightX, x);
	if (g_viewLightX < x) {
		magnitudeX = 0 - magnitudeX;
	}

	magnitudeY = (uint32_t) Difference(g_viewLightY, y);
	if (g_viewLightY < y) {
		magnitudeY = 0 - magnitudeY;
	}

	magnitudeZ = (uint32_t) Difference(g_viewLightZ, z);
	if (g_viewLightZ < z) {
		magnitudeZ = 0 - magnitudeZ;
	}

	bits = magnitudeX | magnitudeY | magnitudeZ;
	if (!bits) {
		return 0x7f;
	}

	/* The dot product in 64 bits, shifted right by 16, and the magnitudes scaled to put the
	   highest bit of any at bit 7, the dot product with them. */
	dot += Product(Difference(g_viewLightZ, z), p_face->m_normal[2]);
	dot = (MechU64) PortableSar64(PortableS64(dot), 16);
	scale = PortableBsr(bits) - 7;
	if (scale > 0) {
		magnitudeX >>= scale;
		magnitudeY >>= scale;
		magnitudeZ >>= scale;
		dot = (MechU64) PortableSar64(PortableS64(dot), scale);
	}
	else if (scale < 0) {
		magnitudeX <<= -scale;
		magnitudeY <<= -scale;
		magnitudeZ <<= -scale;
		dot <<= -scale;
	}

	/* Divided by the length from the square root table, of the low bytes' squares. */
	squares = (magnitudeX & 0xff) * (magnitudeX & 0xff) + (magnitudeY & 0xff) * (magnitudeY & 0xff) +
			  (magnitudeZ & 0xff) * (magnitudeZ & 0xff);
	return PortableS16((uint16_t) PortableIdiv(PortableS64(dot), g_sqrtTable[squares >> 8]));
}

// Queues a face of a model for drawing, unless it faces away: projects the vertices it hasn't
// yet, clips it to the near plane through the filter hooks (g_renderSettings) and adds the
// polygon to the list being built (g_depthList) with its depth, by g_queuedShapeFlags's rule:
// the average (2), nearest (4) or farthest of its vertices' depths. The back-face test, the
// projection, the clipping walk and the depth rules are __asm blocks; the first keeps the normal's
// z in depth.
void QueueFace(Face* p_face, Vertex* p_vertices)
{
	Vertex* vertex;
	Vertex* first;
	Vertex* previous;
	QueuedPolygon* poly;
	uint8_t* index;
	uint16_t count;
	uint8_t andCodes;
	uint8_t clipped;
	uint8_t firstClipped;
	uint8_t previousClipped;
	MechU64 dot;
	MechS64 last;
	int32_t depth;
	int32_t i;

	g_facesTried++;

	/* Faces of three vertices or more face away when the eyepoint's offset from the first one
	   has a dot product with the normal that isn't negative. The flags (jl) test the exact sum
	   of the first two products, which wraps at 64 bits, and the third. */
	if (p_face->m_indexCount >= 3) {
		vertex = &p_vertices[((uint8_t*) p_face)[p_face->m_indexOffset]];
		dot = Product(Difference(vertex->m_worldX, g_viewEyeX), p_face->m_normal[0]) +
			  Product(Difference(vertex->m_worldY, g_viewEyeY), p_face->m_normal[1]);
		last = (MechS64) Difference(vertex->m_worldZ, g_viewEyeZ) * p_face->m_normal[2];
		if (PortableS64(dot) >= -last) {
			return;
		}
	}

	/* The depths of the vertices not yet transformed this frame, with their near (1) and far (2)
	   clip codes. The counts are 16-bit: 0 runs 0x10000 times. */
	g_facesFrontFacing++;
	andCodes = 3;
	count = p_face->m_indexCount;
	index = (uint8_t*) p_face + p_face->m_indexOffset;
	do {
		vertex = &p_vertices[*index];
		if (!(vertex->m_flags & 4)) {
			uint8_t codes = 0;

			g_verticesTransformed++;
			depth = ViewRow(g_viewProjZ0, g_viewProjZ1, g_viewProjZ2, vertex);
			vertex->m_depth = (uint32_t) depth;
			vertex->m_flags |= 4;
			if (depth < g_viewNear) {
				codes |= 1;
			}
			if (depth > g_viewFar) {
				codes |= 2;
			}

			vertex->m_flags = (uint8_t) ((vertex->m_flags & 0xfc) | codes);
		}

		andCodes &= vertex->m_flags;
		index++;
	} while (--count);

	if (andCodes == 1 || andCodes == 2) {
		return;
	}

	/* The clipping walk: each vertex in front of the near plane, and each crossing of it. */
	g_polygonOrCodes = 0;
	g_polygonAndCodes = 0xf;
	g_polygonPointCount = 0;
	index = (uint8_t*) p_face + p_face->m_indexOffset;
	count = p_face->m_indexCount;
	first = previous = &p_vertices[*index];
	firstClipped = previousClipped = first->m_flags & 1;
	if (!firstClipped) {
		g_renderSettings.m_projectVertex(GetViewVertex(first));
	}

	while (--count) {
		index++;
		vertex = &p_vertices[*index];
		clipped = vertex->m_flags & 1;
		if (clipped != previousClipped) {
			g_renderSettings.m_projectVertex(ClipEdgeToNearPlane(previous, vertex));
		}

		previous = vertex;
		previousClipped = clipped;
		if (!clipped) {
			g_renderSettings.m_projectVertex(GetViewVertex(previous));
		}
	}

	if (previousClipped != firstClipped) {
		g_renderSettings.m_projectVertex(ClipEdgeToNearPlane(previous, first));
	}

	if ((g_polygonPointCount < 3 && p_face->m_indexCount > 2) || g_polygonAndCodes) {
		return;
	}

	poly = (QueuedPolygon*) AllocQueuedPolygon();
	poly->m_face = p_face;
	g_polygonPointCursor = g_drawBufferBottom;
	poly->m_count = (int16_t) g_polygonPointCount;

	/* The loops run their count (loop) times: the polygon has a point, as the projection hook
	   has added one whenever it cleared g_polygonAndCodes. */
	PORTABLE_ASSERT(g_polygonPointCount > 0);
	if (g_queuedShapeFlags & 2) {
		uint32_t sum = 0;

		for (i = 0; i < g_polygonPointCount; i++) {
			sum += (uint32_t) g_polygonPoints[i]->m_z;
		}

		/* The sum is unsigned (xor edx, edx), and the count a word. */
		depth = PortableIdiv(sum, (uint16_t) g_polygonPointCount);
	}
	else if (g_queuedShapeFlags & 4) {
		depth = 0x7fffffff;
		for (i = 0; i < g_polygonPointCount; i++) {
			if (depth > g_polygonPoints[i]->m_z) {
				depth = g_polygonPoints[i]->m_z;
			}
		}
	}
	else {
		depth = -0x7fffffff;
		for (i = 0; i < g_polygonPointCount; i++) {
			if (depth < g_polygonPoints[i]->m_z) {
				depth = g_polygonPoints[i]->m_z;
			}
		}
	}

	if (g_queuedShapeFlags & 1) {
		depth |= 0x40000000;
	}

	poly->m_depth = depth;
	memcpy(g_polygonPointCursor, g_polygonPoints, g_polygonPointCount * sizeof(g_polygonPoints[0]));
	g_polygonPointCursor += g_polygonPointCount * sizeof(g_polygonPoints[0]);
	g_drawBufferBottom = g_polygonPointCursor;
	poly->m_flags = (uint16_t) g_renderSettings.m_drawFace(p_face, p_vertices, p_face->m_color, depth);
	if (g_depthEntryCount < g_depthListCapacity) {
		g_polygonsQueued++;
		if (g_polygonPointCount > 1) {
			g_polygonCount++;
		}

		g_depthList[g_depthEntryCount].m_poly = poly;
		g_depthList[g_depthEntryCount].m_depth = depth;
		g_depthEntryCount++;
	}
	else {
		g_queueHasRoom = 0;
	}
}
