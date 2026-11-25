/* Depth-sorted drawing: the shapes of a list or a scene tree queue their polygons in the draw
   buffer (AllocQueuedPolygon) with their depths, the queue is sorted farthest first and drawn,
   clipping each polygon to the screen if a vertex lies too far outside it. */
#include "depthsort.h"

#include "crossdiv.h"
#include "decomp.h"
#include "eyepoint.h"
#include "face.h"
#include "fixedmul.h"
#include "lerp.h"
#include "object.h"
#include "objectanim.h"
#include "polydraw.h"
#include "projectedvertex.h"
#include "queuedpolygon.h"
#include "recordstacks.h"
#include "scaledelta.h"
#include "shape.h"
#include "shapegeom.h"
#include "types.h"
#include "vertex.h"
#include "view.h"

#include <string.h>

// The number of entries in the list being built.
int32_t g_depthEntryCount = 0;

int32_t g_polygonCount = 0;

int32_t g_maxPolygons = 0;

int32_t g_shapesDrawn;

// The depth the shapes are queued at.
int32_t g_queueDepth;

// The list being built: g_depthQueue or g_drawList.
DepthEntry* g_depthList;

// The flags of the shape being queued.
uint32_t g_queuedShapeFlags;

int32_t g_shapesConsidered;

// Selects the shape's level of detail for p_depth (the first model whose distance, scaled by the
// eyepoint, lies beyond it, or the last) and queues its faces. Returns 1 if it has no model.
int32_t QueueShapeLod(Shape* p_shape, int32_t p_depth)
{
	Model* model;
	Face* face;
	Model* cursor;
	Vertex* vertices;
	Vertex* vertex;
	int32_t n;
	int32_t i;
	Model* prev;
	Shape* shape;

	model = NULL;
	prev = NULL;
	shape = p_shape;
	if (!shape) {
		return 1;
	}

	cursor = shape->m_models;
	if (!cursor) {
		return 1;
	}

	if (!cursor->m_key) {
		model = cursor;
	}
	else {
		prev = cursor;
		while (cursor) {
			if (FixedMul16(g_eyepoint->m_detailScale, cursor->m_key) > p_depth) {
				model = prev;
				break;
			}

			prev = cursor;
			cursor = cursor->m_next;
		}
	}

	if (!model) {
		model = prev;
	}

	if (!model) {
		return 1;
	}

	shape->m_model = model;
	vertices = (Vertex*) (model + 1);
	if (shape->m_transformCount != model->m_transformCount) {
		TransformShapeModel(shape);
	}

	vertex = vertices;
	i = n = model->m_vertexCount;
	while (i--) {
		vertex->m_projection = 0;
		vertex->m_flags &= 0xfb;
		vertex++;
	}

	face = (Face*) (model->m_faceOffset + (uint8_t*) model);
	i = n = model->m_faceCount;
	while (i--) {
		QueueFace(face, vertices);
		face++;
		if (!g_queueHasRoom) {
			break;
		}
	}

	return 0;
}

// Sorts the entries from p_first to p_last, farthest first (a quicksort).
void SortDepthEntries(DepthEntry* p_first, DepthEntry* p_last)
{
	int32_t pivot;
	DepthEntry* lo;
	DepthEntry* hi;

	if (p_last <= p_first) {
		return;
	}

	lo = p_first;
	hi = p_last + 1;
	pivot = lo->m_depth;
	for (;;) {
		do {
			lo++;
		} while (lo->m_depth > pivot);

		do {
			hi--;
		} while (hi->m_depth < pivot);

		if (hi <= lo) {
			break;
		}

		{
			QueuedPolygon* poly = lo->m_poly;
			lo->m_poly = hi->m_poly;
			hi->m_poly = poly;
		}
		{
			int32_t depth = lo->m_depth;
			lo->m_depth = hi->m_depth;
			hi->m_depth = depth;
		}
	}

	lo = p_first;
	{
		QueuedPolygon* poly = lo->m_poly;
		lo->m_poly = hi->m_poly;
		hi->m_poly = poly;
	}
	{
		int32_t depth = lo->m_depth;
		lo->m_depth = hi->m_depth;
		hi->m_depth = depth;
	}

	if (hi - 1 > p_first) {
		SortDepthEntries(p_first, hi - 1);
	}

	if (hi + 1 < p_last) {
		SortDepthEntries(hi + 1, p_last);
	}
}

// Draws the shapes of the list p_root heads, farthest first. Shapes flagged 0x100 are queued
// whole and expanded after the sort (DrawDepthQueue).
void DrawShapeList(Shape* p_root)
{
	Shape* shape;

	ResetDrawBuffer();
	if (!p_root || !p_root->m_next || p_root->m_prev == p_root->m_next) {
		return;
	}

	g_depthEntryCount = 0;
	g_polygonCount = 0;
	g_queueHasRoom = 1;
	g_depthList = g_depthQueue;
	for (shape = p_root->m_next; shape; shape = shape->m_next) {
		g_shapesConsidered++;
		if (!g_renderSettings.m_shapeFilter(shape)) {
			g_shapesDrawn++;
			g_queuedShapeFlags = shape->m_flags;
			if (g_queuedShapeFlags & 0x100) {
				shape->m_flags |= 0x8000;
				g_depthList[g_depthEntryCount].m_poly = (QueuedPolygon*) shape;
				g_depthList[g_depthEntryCount].m_depth = g_queueDepth;
				g_depthEntryCount++;
			}
			else {
				QueueShapeLod(shape, g_queueDepth);
				if (!g_queueHasRoom || g_polygonCount >= g_maxPolygons) {
					break;
				}
			}
		}
	}

	DrawDepthQueue();
}

// Sorts the queue DrawShapeList built, moves its polygons to g_drawList in order, expanding
// each whole shape into its own sorted run of polygons, and draws them.
void DrawDepthQueue(void)
{
	int32_t count;
	int32_t i;
	int32_t start;

	start = 0;
	if (g_depthEntryCount > 1) {
		SortDepthEntries(g_depthQueue, &g_depthQueue[g_depthEntryCount - 1]);
	}

	count = g_depthEntryCount;
	g_depthEntryCount = 0;
	g_depthList = g_drawList;
	for (i = 0; i < count; i++) {
		if (!(g_depthQueue[i].m_poly->m_count & 0x8000)) {
			memcpy(&g_drawList[g_depthEntryCount], &g_depthQueue[i], sizeof(DepthEntry));
			g_depthEntryCount++;
		}
		else if (g_queueHasRoom) {
			start = g_depthEntryCount;
			QueueShapeLod((Shape*) g_depthQueue[i].m_poly, g_depthQueue[i].m_depth);
			if (!g_queueHasRoom) {
				break;
			}

			if (g_depthEntryCount - start > 1) {
				SortDepthEntries(&g_drawList[start], &g_drawList[g_depthEntryCount - 1]);
			}
		}

		if (g_polygonCount >= g_maxPolygons) {
			break;
		}
	}

	for (i = 0; i < g_depthEntryCount; i++) {
		DrawQueuedPolygon(g_drawList[i].m_poly);
	}
}

// Draws the shapes of the scene tree p_root, farthest first.
void DrawObjTreeShapes(SceneObject* p_root)
{
	int32_t i;

	ResetDrawBuffer();
	g_depthEntryCount = 0;
	g_polygonCount = 0;
	g_queueHasRoom = 1;
	g_depthList = g_drawList;
	QueueObjTree(p_root);
	if (g_depthEntryCount > 1) {
		SortDepthEntries(g_drawList, &g_drawList[g_depthEntryCount - 1]);
	}

	for (i = 0; i < g_depthEntryCount; i++) {
		DrawQueuedPolygon(g_drawList[i].m_poly);
	}
}

// Queues the polygons of p_object's shape and of its descendants' shapes.
void QueueObjTree(SceneObject* p_object)
{
	SceneObject* child;
	Shape* shape;

	if (!p_object || !g_queueHasRoom || g_polygonCount >= g_maxPolygons) {
		return;
	}

	shape = p_object->m_shape;
	if (shape) {
		g_shapesConsidered++;
		if (!(shape->m_flags & 0x1000) && !g_renderSettings.m_shapeFilter(shape)) {
			g_shapesDrawn++;
			QueueShapeLod(shape, g_queueDepth);
		}
	}

	for (child = p_object->m_firstChild; child; child = child->m_nextSibling) {
		QueueObjTree(child);
	}
}

void DrawShapeListFrom(Shape* p_root, Eyepoint* p_eyepoint)
{
	SelectEyepoint(p_eyepoint);
	DrawShapeList(p_root);
}

// Draws a queued polygon, clipping it to the screen first if a vertex lies far outside it.
void DrawQueuedPolygon(QueuedPolygon* p_poly)
{
	ProjectedVertex* vertex;
	int32_t count;
	ProjectedVertex** vertices;
	uint32_t points[10 * 6];
	uint32_t* point;
	int32_t i;
	int32_t x;
	int32_t y;

	count = p_poly->m_count;
	vertices = (ProjectedVertex**) (p_poly + 1);
	point = points;
	i = count;
	while (i--) {
		vertex = *vertices;
		vertices++;
		x = vertex->m_screenX;
		y = vertex->m_screenY;
		if (x <= -0x4000 || x >= 0x3fff || y <= -0x4000 || y >= 0x3fff) {
			count = ClipPolygonToScreen(p_poly, points);
			break;
		}

		point[0] = x;
		point[1] = y;
		point[3] = vertex->m_u;
		point[4] = vertex->m_v;
		point[2] = 0;
		point[5] = vertex->m_z;
		point += 6;
	}

	if (count > 0) {
		DrawPolygonOrLine(count, points, p_poly->m_flags);
	}
}

// Clips a queued polygon to the screen (right, left, top and bottom in turn) and stores the
// result in p_points, six dwords per vertex. Returns the number of vertices.
int32_t ClipPolygonToScreen(QueuedPolygon* p_poly, uint32_t* p_points)
{
	ProjectedVertex* cur;
	int32_t bottom;
	int32_t right;
	ProjectedVertex* out;
	ProjectedVertex* prev;
	int32_t top;
	ProjectedVertex* src;
	int32_t n;
	ProjectedVertex** vertices;
	ProjectedVertex* in;
	ProjectedVertex bufferA[20];
	ProjectedVertex clip;
	ProjectedVertex bufferB[20];
	int32_t i;
	int32_t left;
	int32_t count;

	right = bottom = 0x3fff;
	left = top = -0x4000;
	out = bufferA;
	count = p_poly->m_count;
	vertices = (ProjectedVertex**) (p_poly + 1);
	i = count;
	while (i--) {
		src = *vertices;
		vertices++;
		*out = *src;
		out++;
	}

	n = 0;
	in = bufferA;
	out = bufferB;
	prev = &in[count - 1];
	for (i = 0; i < count; i++) {
		cur = &in[i];
		if (cur->m_screenX <= right) {
			if (prev->m_screenX <= right) {
				*out = *cur;
				out++;
				n++;
			}
			else {
				ClipEdgeToColumn(prev, cur, right, &clip);
				*out = clip;
				out++;
				n++;
				*out = *cur;
				out++;
				n++;
			}
		}
		else {
			if (prev->m_screenX <= right) {
				ClipEdgeToColumn(prev, cur, right, &clip);
				*out = clip;
				out++;
				n++;
			}
		}

		prev = cur;
	}

	count = n;
	n = 0;
	in = bufferB;
	out = bufferA;
	prev = &in[count - 1];
	for (i = 0; i < count; i++) {
		cur = &in[i];
		if (cur->m_screenX >= left) {
			if (prev->m_screenX >= left) {
				*out = *cur;
				out++;
				n++;
			}
			else {
				ClipEdgeToColumn(prev, cur, left, &clip);
				*out = clip;
				out++;
				n++;
				*out = *cur;
				out++;
				n++;
			}
		}
		else {
			if (prev->m_screenX >= left) {
				ClipEdgeToColumn(prev, cur, left, &clip);
				*out = clip;
				out++;
				n++;
			}
		}

		prev = cur;
	}

	count = n;
	n = 0;
	in = bufferA;
	out = bufferB;
	prev = &in[count - 1];
	for (i = 0; i < count; i++) {
		cur = &in[i];
		if (cur->m_screenY >= top) {
			if (prev->m_screenY >= top) {
				*out = *cur;
				out++;
				n++;
			}
			else {
				ClipEdgeToRow(prev, cur, top, &clip);
				*out = clip;
				out++;
				n++;
				*out = *cur;
				out++;
				n++;
			}
		}
		else {
			if (prev->m_screenY >= top) {
				ClipEdgeToRow(prev, cur, top, &clip);
				*out = clip;
				out++;
				n++;
			}
		}

		prev = cur;
	}

	count = n;
	n = 0;
	in = bufferB;
	out = bufferA;
	prev = &in[count - 1];
	for (i = 0; i < count; i++) {
		cur = &in[i];
		if (cur->m_screenY <= bottom) {
			if (prev->m_screenY <= bottom) {
				*out = *cur;
				out++;
				n++;
			}
			else {
				ClipEdgeToRow(prev, cur, bottom, &clip);
				*out = clip;
				out++;
				n++;
				*out = *cur;
				out++;
				n++;
			}
		}
		else {
			if (prev->m_screenY <= bottom) {
				ClipEdgeToRow(prev, cur, bottom, &clip);
				*out = clip;
				out++;
				n++;
			}
		}

		prev = cur;
	}

	for (cur = bufferA, i = n; i--; cur++, p_points += 6) {
		p_points[0] = cur->m_screenX;
		p_points[1] = cur->m_screenY;
		p_points[3] = cur->m_u;
		p_points[4] = cur->m_v;
		p_points[2] = 0;
		p_points[5] = cur->m_z;
	}

	return n;
}

// Returns the depth where the edge from (p_a0, p_z0) to (p_a1, p_z1), in view space, crosses
// the screen column (p_isX) or row p_edge.
int32_t GetEdgeCrossingDepth(int32_t p_a0, int32_t p_a1, int32_t p_edge, int32_t p_isX, int32_t p_z0, int32_t p_z1)
{
	int32_t dz;
	int32_t t;
	int32_t z;
	int32_t da;

	da = p_a1 - p_a0;
	dz = p_z1 - p_z0;
	if (dz < 0) {
		da = -da;
		dz = -dz;
		p_a0 = p_a1;
		p_z0 = p_z1;
	}

	if (p_isX) {
		p_edge = UnprojectCoordinate(p_edge, g_viewCenterX, dz, g_viewShiftX);
	}
	else {
		p_edge = UnprojectCoordinate(g_viewCenterY, p_edge, dz, g_viewShiftY);
	}

	t = p_edge - da;
	if (!t) {
		z = p_z0;
	}
	else {
		z = CrossDiv(p_a0, p_z0, da, dz, t);
	}

	return z;
}

// Stores in p_out the point where the edge from p_a to p_b crosses the screen column p_x.
void ClipEdgeToColumn(ProjectedVertex* p_a, ProjectedVertex* p_b, int32_t p_x, ProjectedVertex* p_out)
{
	int32_t z;
	int32_t dx;
	int32_t dy;

	p_out->m_z = GetEdgeCrossingDepth(p_a->m_x, p_b->m_x, p_x, 1, p_a->m_z, p_b->m_z);
	z = p_out->m_z;
	p_out->m_screenX = p_x;
	p_out->m_screenY = Lerp(p_a->m_screenX, p_b->m_screenX, p_x, p_a->m_screenY, p_b->m_screenY);
	p_out->m_x = UnprojectCoordinate(p_x, g_viewCenterX, z, g_viewShiftX);
	p_out->m_y = UnprojectCoordinate(g_viewCenterY, p_out->m_screenY, z, g_viewShiftY);

	dx = p_a->m_x - p_b->m_x;
	if (dx < 0) {
		dx = -dx;
	}

	dy = p_a->m_y - p_b->m_y;
	if (dy < 0) {
		dy = -dy;
	}

	if (dx > dy) {
		p_out->m_u = Lerp(p_a->m_x, p_b->m_x, p_out->m_x, p_a->m_u, p_b->m_u);
		p_out->m_v = Lerp(p_a->m_x, p_b->m_x, p_out->m_x, p_a->m_v, p_b->m_v);
	}
	else if (dy) {
		p_out->m_u = Lerp(p_a->m_y, p_b->m_y, p_out->m_y, p_a->m_u, p_b->m_u);
		p_out->m_v = Lerp(p_a->m_y, p_b->m_y, p_out->m_y, p_a->m_v, p_b->m_v);
	}
	else {
		p_out->m_u = (p_a->m_u + p_b->m_u) >> 1;
		p_out->m_v = (p_a->m_v + p_b->m_v) >> 1;
	}
}

// Stores in p_out the point where the edge from p_a to p_b crosses the screen row p_y.
void ClipEdgeToRow(ProjectedVertex* p_a, ProjectedVertex* p_b, int32_t p_y, ProjectedVertex* p_out)
{
	int32_t z;
	int32_t dx;
	int32_t dy;

	p_out->m_z = GetEdgeCrossingDepth(p_a->m_y, p_b->m_y, p_y, 0, p_a->m_z, p_b->m_z);
	z = p_out->m_z;
	p_out->m_screenX = Lerp(p_a->m_screenY, p_b->m_screenY, p_y, p_a->m_screenX, p_b->m_screenX);
	p_out->m_screenY = p_y;
	p_out->m_x = UnprojectCoordinate(p_out->m_screenX, g_viewCenterX, z, g_viewShiftX);
	p_out->m_y = UnprojectCoordinate(g_viewCenterY, p_y, z, g_viewShiftY);

	dx = p_a->m_x - p_b->m_x;
	if (dx < 0) {
		dx = -dx;
	}

	dy = p_a->m_y - p_b->m_y;
	if (dy < 0) {
		dy = -dy;
	}

	if (dy > dx) {
		p_out->m_u = Lerp(p_a->m_y, p_b->m_y, p_out->m_y, p_a->m_u, p_b->m_u);
		p_out->m_v = Lerp(p_a->m_y, p_b->m_y, p_out->m_y, p_a->m_v, p_b->m_v);
	}
	else if (dx) {
		p_out->m_u = Lerp(p_a->m_x, p_b->m_x, p_out->m_x, p_a->m_u, p_b->m_u);
		p_out->m_v = Lerp(p_a->m_x, p_b->m_x, p_out->m_x, p_a->m_v, p_b->m_v);
	}
	else {
		p_out->m_u = (p_a->m_u + p_b->m_u) >> 1;
		p_out->m_v = (p_a->m_v + p_b->m_v) >> 1;
	}
}
