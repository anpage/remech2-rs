#ifndef DEPTHSORT_H
#define DEPTHSORT_H

#include "types.h"

struct SceneObject;
struct ProjectedVertex;
struct Eyepoint;
struct QueuedPolygon;
struct Shape;

// An entry of the depth-sorted draw lists (g_depthQueue, g_drawList): a polygon, or a
// shape whose polygons are queued once the list is sorted (QueuedPolygon::m_count bit 15).
// SIZE 0x8
typedef struct DepthEntry {
	struct QueuedPolygon* m_poly; // 0x00
	MechS32 m_depth;              // 0x04
} DepthEntry;

// The functions and globals of depthsort.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_depthEntryCount;
	extern MechS32 g_polygonCount;
	extern MechS32 g_maxPolygons;
	extern MechS32 g_shapesDrawn;
	extern MechS32 g_queueDepth;
	extern DepthEntry* g_depthList;
	extern MechU32 g_queuedShapeFlags;
	extern MechS32 g_shapesConsidered;

	MechS32 QueueShapeLod(struct Shape* p_shape, MechS32 p_depth);
	void SortDepthEntries(DepthEntry* p_first, DepthEntry* p_last);
	void DrawShapeList(struct Shape* p_root);
	void DrawDepthQueue(void);
	void DrawObjTreeShapes(struct SceneObject* p_root);
	void QueueObjTree(struct SceneObject* p_object);
	void DrawShapeListFrom(struct Shape* p_root, struct Eyepoint* p_eyepoint);
	void DrawQueuedPolygon(struct QueuedPolygon* p_poly);
	MechS32 ClipPolygonToScreen(struct QueuedPolygon* p_poly, MechU32* p_points);
	MechS32 GetEdgeCrossingDepth(MechS32 p_a0, MechS32 p_a1, MechS32 p_edge, MechS32 p_isX, MechS32 p_z0, MechS32 p_z1);
	void ClipEdgeToColumn(
		struct ProjectedVertex* p_a,
		struct ProjectedVertex* p_b,
		MechS32 p_x,
		struct ProjectedVertex* p_out
	);
	void ClipEdgeToRow(
		struct ProjectedVertex* p_a,
		struct ProjectedVertex* p_b,
		MechS32 p_y,
		struct ProjectedVertex* p_out
	);

#ifdef __cplusplus
}
#endif

#endif // DEPTHSORT_H
