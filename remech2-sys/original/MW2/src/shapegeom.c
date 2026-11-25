/* In the original, TransformShapeCenter, SolvePlaneY, ApproximateShapeDistance,
   ComputeTriangleNormal, DivDifference17 and RayShapeDistance are C functions with __asm bodies,
   and TransformModel has an __asm block. The portable C here replaces TransformShapeCenter's __asm
   block and the others' whole bodies, and wraps where standard C overflows. */
#include "shapegeom.h"

#include "clock.h"
#include "face.h"
#include "portable.h"
#include "ray.h"
#include "rendersettings.h"
#include "shape.h"
#include "transform.h"
#include "types.h"
#include "vertex.h"

// The 64-bit product of two 32-bit values, as imul leaves it in edx:eax. Sums of products wrap,
// like the add/adc chains.
static uint64_t Product(int32_t p_a, int32_t p_b)
{
	return (uint64_t) ((int64_t) p_a * p_b);
}

// sub: wraps.
static int32_t Difference(int32_t p_a, int32_t p_b)
{
	return PortableS32((uint32_t) p_a - (uint32_t) p_b);
}

// neg: INT_MIN stays INT_MIN.
static int32_t Negate(int32_t p_value)
{
	return PortableS32(0 - (uint32_t) p_value);
}

// Row p_row of p_matrix's 2.29 rotation times (p_x, p_y, p_z), shifted right by 29, rounded and
// offset by p_offset.
static int32_t TransformRow(Matrix* p_matrix, int32_t p_row, int32_t p_x, int32_t p_y, int32_t p_z, int32_t p_offset)
{
	int32_t* row = p_matrix->m_rows[p_row];
	uint64_t sum = Product(row[0], p_x) + Product(row[1], p_y) + Product(row[2], p_z);

	return PortableS32(PortableShrdRound(sum, 29) + (uint32_t) p_offset);
}

// Transforms p_model's vertices (their positions into m_worldX-m_worldZ) and face normals by
// p_matrix. The products are an __asm block.
void TransformModel(Model* p_model, Matrix* p_matrix)
{
	Vertex* vertex = (Vertex*) (p_model + 1);
	Face* face = (Face*) ((uint8_t*) p_model + p_model->m_faceOffset);
	uint16_t count;

	/* The counts are 16-bit: 0 runs 0x10000 times. */
	count = (uint16_t) p_model->m_vertexCount;
	do {
		int32_t x = vertex->m_modelX;
		int32_t y = vertex->m_modelY;
		int32_t z = vertex->m_modelZ;

		vertex->m_worldX = TransformRow(p_matrix, 0, x, y, z, p_matrix->m_rows[3][0]);
		vertex->m_worldY = TransformRow(p_matrix, 1, x, y, z, p_matrix->m_rows[3][1]);
		vertex->m_worldZ = TransformRow(p_matrix, 2, x, y, z, p_matrix->m_rows[3][2]);
		vertex++;
	} while (--count);

	count = (uint16_t) p_model->m_faceCount;
	do {
		int32_t x = face->m_modelNormalX;
		int32_t y = face->m_modelNormalY;
		int32_t z = face->m_modelNormalZ;

		face->m_normal[0] = TransformRow(p_matrix, 0, x, y, z, 0);
		face->m_normal[1] = TransformRow(p_matrix, 1, x, y, z, 0);
		face->m_normal[2] = TransformRow(p_matrix, 2, x, y, z, 0);
		face++;
	} while (--count);
}

// Transforms the shape's position (m_modelCenterX-Z) by p_matrix into m_centerX-Z, and bumps its
// transform count (m_transformCount). The products are an __asm block.
void TransformShapeCenter(struct Shape* p_shape, Matrix* p_matrix)
{
	p_shape->m_flags &= ~0x200;
	p_shape->m_centerX = TransformRow(
		p_matrix,
		0,
		p_shape->m_modelCenterX,
		p_shape->m_modelCenterY,
		p_shape->m_modelCenterZ,
		p_matrix->m_rows[3][0]
	);
	p_shape->m_centerY = TransformRow(
		p_matrix,
		1,
		p_shape->m_modelCenterX,
		p_shape->m_modelCenterY,
		p_shape->m_modelCenterZ,
		p_matrix->m_rows[3][1]
	);
	p_shape->m_centerZ = TransformRow(
		p_matrix,
		2,
		p_shape->m_modelCenterX,
		p_shape->m_modelCenterY,
		p_shape->m_modelCenterZ,
		p_matrix->m_rows[3][2]
	);

	p_shape->m_transformCount++;
}

// Transforms a shape and each of its models by p_matrix.
void TransformShape(struct Shape* p_shape, Matrix* p_matrix)
{
	Model* model;

	TransformShapeCenter(p_shape, p_matrix);
	for (model = p_shape->m_models; model; model = model->m_next) {
		TransformModel(model, p_matrix);
		model->m_transformCount = p_shape->m_transformCount;
	}
}

// Solves the plane p_normalX * x + p_normalY * y + p_normalZ * z + p_distance = 0 for y at
// (p_dx, p_dz): (p_normalX * p_dx + p_normalZ * p_dz + p_distance) / p_normalY, in 64 bits.
int32_t SolvePlaneY(
	int32_t p_normalX,
	int32_t p_normalY,
	int32_t p_normalZ,
	int32_t p_distance,
	int32_t p_dx,
	int32_t p_dz
)
{
	/* p_distance is added unsigned (adc edx, 0), and the sum wraps at 64 bits. */
	uint64_t sum = Product(p_normalX, p_dx) + Product(p_normalZ, p_dz) + (uint32_t) p_distance;

	return PortableIdiv(PortableS64(sum), p_normalY);
}

// Returns an approximate distance from (p_x, p_y, p_z) to the shape's center, (4 * the largest +
// the others) / 4 of the offsets, or 0x7fffffff outside its bounding sphere.
int32_t ApproximateShapeDistance(struct Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z)
{
	int32_t deltaX = Difference(p_shape->m_centerX, p_x);
	int32_t deltaY = Difference(p_shape->m_centerY, p_y);
	int32_t deltaZ = Difference(p_shape->m_centerZ, p_z);
	int32_t radius = p_shape->m_radius;
	int32_t low = Negate(radius);
	int32_t largest;
	int32_t second;
	int32_t third;
	int32_t swap;

	if (deltaX > radius || deltaX < low || deltaY > radius || deltaY < low || deltaZ > radius || deltaZ < low) {
		return 0x7fffffff;
	}

	/* The squares compare unsigned (sub/sbb, jae). */
	if (Product(radius, radius) < Product(deltaZ, deltaZ) + Product(deltaX, deltaX) + Product(deltaY, deltaY)) {
		return 0x7fffffff;
	}

	/* The magnitudes compare signed: neg leaves INT_MIN negative. */
	largest = deltaX < 0 ? Negate(deltaX) : deltaX;
	second = deltaY < 0 ? Negate(deltaY) : deltaY;
	if (largest < second) {
		swap = largest;
		largest = second;
		second = swap;
	}

	third = deltaZ < 0 ? Negate(deltaZ) : deltaZ;
	if (largest < third) {
		swap = largest;
		largest = third;
		third = swap;
	}

	return PortableS32((((uint32_t) largest << 2) + (uint32_t) second + (uint32_t) third) >> 2);
}

// The normal of the triangle (p_x0, p_y0, p_z0), (p_x1, p_y1, p_z1), (p_x2, p_y2, p_z2): the cross
// product of its edges, scaled to 2.29 fixed point; returns the scale's exponent. A degenerate
// triangle gets the normal (-1, -1, -1) and 0. The products and the scaling are __asm blocks.
int32_t ComputeTriangleNormal(
	int32_t p_x0,
	int32_t p_y0,
	int32_t p_z0,
	int32_t p_x1,
	int32_t p_y1,
	int32_t p_z1,
	int32_t p_x2,
	int32_t p_y2,
	int32_t p_z2,
	int32_t* p_nx,
	int32_t* p_ny,
	int32_t* p_nz
)
{
	int64_t normal[3];
	uint32_t values[3];
	int32_t quotients[3];
	uint32_t high = 0;
	uint32_t low = 0;
	uint32_t squares = 0;
	int32_t divisor;
	int32_t shift;
	int32_t scale;
	int32_t i;

	/* The cross product of the edges, each component a 64-bit difference of products (which
	   can't overflow), and the bits of their magnitudes. */
	normal[0] = (int64_t) Difference(p_y1, p_y0) * Difference(p_z2, p_z1) -
				(int64_t) Difference(p_y2, p_y1) * Difference(p_z1, p_z0);
	normal[1] = (int64_t) Difference(p_z1, p_z0) * Difference(p_x2, p_x1) -
				(int64_t) Difference(p_z2, p_z1) * Difference(p_x1, p_x0);
	normal[2] = (int64_t) Difference(p_x1, p_x0) * Difference(p_y2, p_y1) -
				(int64_t) Difference(p_x2, p_x1) * Difference(p_y1, p_y0);
	for (i = 0; i < 3; i++) {
		uint64_t magnitude = (uint64_t) (normal[i] < 0 ? -normal[i] : normal[i]);

		high |= (uint32_t) (magnitude >> 32);
		low |= (uint32_t) magnitude;
	}

	/* Scaled to 32 bits with the highest bit at 29 or below: shifted right by the magnitude's
	   bits above 29 (the shift count modulo 32, like the shrd's), or left by those missing. */
	if (high) {
		scale = PortableBsr(high) + 3;
		for (i = 0; i < 3; i++) {
			values[i] = (uint32_t) ((uint64_t) normal[i] >> (scale & 31));
		}

		shift = scale + 29;
	}
	else if (low) {
		shift = PortableBsr(low);
		scale = 29 - shift;
		for (i = 0; i < 3; i++) {
			if (scale < 0) {
				values[i] = (uint32_t) ((uint64_t) normal[i] >> -scale);
			}
			else {
				values[i] = (uint32_t) normal[i] << scale;
			}
		}
	}
	else {
		*p_nx = *p_ny = *p_nz = -1;
		return 0;
	}

	/* Normalized by the square root of the sum of the high words' squares. */
	for (i = 0; i < 3; i++) {
		int32_t word = PortableS16((uint16_t) (values[i] >> 16));

		squares += (uint32_t) (word * word);
	}

	divisor = (uint16_t) g_sqrtTable[squares >> 20];
	for (i = 0; i < 3; i++) {
		quotients[i] = PortableIdiv((int64_t) PortableS32(values[i]) * 0x2000, divisor);
	}

	*p_nx = Negate(quotients[0]);
	*p_ny = Negate(quotients[1]);
	*p_nz = Negate(quotients[2]);
	return shift;
}

// Returns p_value / (p_a - p_b) in 15.17 fixed point, or 0 if p_a and p_b are equal.
int32_t DivDifference17(int32_t p_a, int32_t p_b, int32_t p_value)
{
	int32_t divisor = Difference(p_a, p_b);

	if (!divisor) {
		return 0;
	}

	return PortableIdiv((int64_t) p_value * 0x20000, divisor);
}

// Returns how far along p_ray it passes closest to the shape's center, less the radius, or
// 0x7fffffff when it misses the bounding sphere or ends first; 10 when the ray starts inside it.
int32_t RayShapeDistance(struct Shape* p_shape, Ray* p_ray)
{
	int32_t radius = p_shape->m_radius;
	int32_t deltaX;
	int32_t deltaY;
	int32_t deltaZ;
	uint64_t excess;
	uint64_t partial;
	uint64_t dot;
	int32_t t;

	if (radius <= 0) {
		return 0x7fffffff;
	}

	deltaX = Difference(p_shape->m_centerX, p_ray->m_x0);
	deltaY = Difference(p_shape->m_centerY, p_ray->m_y0);
	deltaZ = Difference(p_shape->m_centerZ, p_ray->m_z0);
	excess = Product(deltaX, deltaX) + Product(deltaY, deltaY) + Product(deltaZ, deltaZ);
	if (excess < Product(radius, radius)) {
		return 10;
	}

	/* The ray misses when the last add of the dot product carries out of 64 bits (adc, jae),
	   whatever the sign. */
	excess -= Product(radius, radius);
	partial = Product(deltaX, p_ray->m_dx) + Product(deltaY, p_ray->m_dy);
	dot = partial + Product(deltaZ, p_ray->m_dz);
	if (dot < partial) {
		return 0x7fffffff;
	}

	t = PortableIdiv(PortableS64(dot), p_ray->m_length);
	if (Product(t, t) < excess) {
		return 0x7fffffff;
	}

	t = Difference(t, radius);
	if (t < 0) {
		t = 0;
	}

	return t >= GetRayLength(p_ray) ? 0x7fffffff : t;
}
