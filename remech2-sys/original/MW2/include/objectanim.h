#ifndef OBJECTANIM_H
#define OBJECTANIM_H

#include "face.h"
#include "path.h"
#include "projectedvertex.h"
#include "reel.h"
#include "resourceref.h"
#include "types.h"
#include "vertex.h"

// An animation file LoadAnimFile has loaded: its id and the base of its animation numbers.
typedef struct AnimFile {
	int32_t m_id;
	int32_t m_base;
} AnimFile;

// The functions and globals of objectanim.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_pathCount;
	extern int32_t g_maxAnimNumber;
	extern int32_t g_animBase;
	extern int32_t g_animFileCount;
	extern int32_t g_directionalLight;
	extern uint8_t g_polygonOrCodes;
	extern uint8_t g_polygonAndCodes;
	extern ProjectedVertex* g_polygonPoints[20];
	extern int32_t g_polygonPointCount;
	extern Path g_paths[0x40];
	extern Reel* g_reels[0x780];
	extern int32_t g_reelMotionError;
	extern uint8_t* g_polygonPointCursor;
	extern int32_t g_facesTried;
	extern int32_t g_facesFrontFacing;
	extern int32_t g_verticesTransformed;
	extern int32_t g_polygonsQueued;
	extern AnimFile g_animFiles[60];

	int32_t ReelMotionTask(int32_t p_event, char* p_data, int32_t p_clock, int32_t p_period);
	int32_t ScaleBySpeedLevel(int32_t p_mode, int32_t p_value);
	int32_t LoadAnimFile(ResourceRef* p_ref);
	int32_t GetAnimBase(void);
	int32_t GetReelMotionSize(void);
	int32_t ColorCycleTask(int32_t p_event, char* p_data, int32_t p_clock, int32_t p_period);
	int32_t SpinTask(int32_t p_event, char* p_data, int32_t p_clock, int32_t p_period);
	int32_t OrbitTask(int32_t p_event, char* p_data, int32_t p_clock, int32_t p_period);
	int32_t AmbientSoundTask(int32_t p_event, char* p_data, int32_t p_clock, int32_t p_period);
	int32_t PathTask(int32_t p_event, char* p_data, int32_t p_clock, int32_t p_period);
	ProjectedVertex* GetViewVertex(Vertex* p_vertex);
	ProjectedVertex* ClipEdgeToNearPlane(Vertex* p_a, Vertex* p_b);
	ProjectedVertex* ProjectVertex(ProjectedVertex* p_vertex);
	int32_t GetFaceShade(Face* p_face, Vertex* p_vertices);
	void QueueFace(Face* p_face, Vertex* p_vertices);

#ifdef __cplusplus
}
#endif

#endif // OBJECTANIM_H
