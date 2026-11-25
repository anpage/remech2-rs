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
typedef struct DepthEntry {
	struct QueuedPolygon* m_poly; // 0x00
	int32_t m_depth;              // 0x04
} DepthEntry;

// The functions and globals of depthsort.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_depthEntryCount;
	extern int32_t g_polygonCount;
	extern int32_t g_maxPolygons;
	extern int32_t g_shapesDrawn;
	extern int32_t g_queueDepth;
	extern DepthEntry* g_depthList;
	extern uint32_t g_queuedShapeFlags;
	extern int32_t g_shapesConsidered;

	int32_t QueueShapeLod(struct Shape* p_shape, int32_t p_depth);
	void SortDepthEntries(DepthEntry* p_first, DepthEntry* p_last);
	void DrawShapeList(struct Shape* p_root);
	void DrawDepthQueue(void);
	void DrawObjTreeShapes(struct SceneObject* p_root);
	void QueueObjTree(struct SceneObject* p_object);
	void DrawShapeListFrom(struct Shape* p_root, struct Eyepoint* p_eyepoint);
	void DrawQueuedPolygon(struct QueuedPolygon* p_poly);
	int32_t ClipPolygonToScreen(struct QueuedPolygon* p_poly, uint32_t* p_points);
	int32_t GetEdgeCrossingDepth(int32_t p_a0, int32_t p_a1, int32_t p_edge, int32_t p_isX, int32_t p_z0, int32_t p_z1);
	void ClipEdgeToColumn(
		struct ProjectedVertex* p_a,
		struct ProjectedVertex* p_b,
		int32_t p_x,
		struct ProjectedVertex* p_out
	);
	void ClipEdgeToRow(
		struct ProjectedVertex* p_a,
		struct ProjectedVertex* p_b,
		int32_t p_y,
		struct ProjectedVertex* p_out
	);

#ifdef __cplusplus
}
#endif

#endif // DEPTHSORT_H
