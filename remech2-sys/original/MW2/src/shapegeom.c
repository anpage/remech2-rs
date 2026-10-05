/* In the original, TransformShapeCenter, SolvePlaneY, ApproximateShapeDistance,
   ComputeTriangleNormal, DivDifference17 and RayShapeDistance are C functions with __asm bodies,
   and TransformModel has an __asm block. The portable C here replaces TransformShapeCenter's __asm
   block and the others' whole bodies, and wraps where standard C overflows. */
#include "shapegeom.h"

#include "clock.h"
#include "decomp.h"
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
static MechU64 Product(MechS32 p_a, MechS32 p_b)
{
	return (MechU64) ((MechS64) p_a * p_b);
}

// sub: wraps.
static MechS32 Difference(MechS32 p_a, MechS32 p_b)
{
	return PortableS32((MechU32) p_a - (MechU32) p_b);
}

// neg: INT_MIN stays INT_MIN.
static MechS32 Negate(MechS32 p_value)
{
	return PortableS32(0 - (MechU32) p_value);
}

// Row p_row of p_matrix's 2.29 rotation times (p_x, p_y, p_z), shifted right by 29, rounded and
// offset by p_offset.
static MechS32 TransformRow(Matrix* p_matrix, MechS32 p_row, MechS32 p_x, MechS32 p_y, MechS32 p_z, MechS32 p_offset)
{
	MechS32* row = p_matrix->m_rows[p_row];
	MechU64 sum = Product(row[0], p_x) + Product(row[1], p_y) + Product(row[2], p_z);

	return PortableS32(PortableShrdRound(sum, 29) + (MechU32) p_offset);
}

// Transforms p_model's vertices (their positions into m_worldX-m_worldZ) and face normals by
// p_matrix. The products are an __asm block.
// Stack-slot permutation of the locals.
// FUNCTION: MW2 0x10039a30
void TransformModel(Model* p_model, Matrix* p_matrix)
{
	Vertex* vertex = (Vertex*) (p_model + 1);
	Face* face = (Face*) ((MechU8*) p_model + p_model->m_faceOffset);
	MechU16 count;

	/* The counts are 16-bit: 0 runs 0x10000 times. */
	count = (MechU16) p_model->m_vertexCount;
	do {
		MechS32 x = vertex->m_modelX;
		MechS32 y = vertex->m_modelY;
		MechS32 z = vertex->m_modelZ;

		vertex->m_worldX = TransformRow(p_matrix, 0, x, y, z, p_matrix->m_rows[3][0]);
		vertex->m_worldY = TransformRow(p_matrix, 1, x, y, z, p_matrix->m_rows[3][1]);
		vertex->m_worldZ = TransformRow(p_matrix, 2, x, y, z, p_matrix->m_rows[3][2]);
		vertex++;
	} while (--count);

	count = (MechU16) p_model->m_faceCount;
	do {
		MechS32 x = face->m_modelNormalX;
		MechS32 y = face->m_modelNormalY;
		MechS32 z = face->m_modelNormalZ;

		face->m_normal[0] = TransformRow(p_matrix, 0, x, y, z, 0);
		face->m_normal[1] = TransformRow(p_matrix, 1, x, y, z, 0);
		face->m_normal[2] = TransformRow(p_matrix, 2, x, y, z, 0);
		face++;
	} while (--count);
}

// Transforms the shape's position (m_modelCenterX-Z) by p_matrix into m_centerX-Z, and bumps its
// transform count (m_transformCount). The products are an __asm block.
// FUNCTION: MW2 0x10039b94
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
// FUNCTION: MW2 0x10039c36
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
// FUNCTION: MW2 0x10039c96
MechS32 SolvePlaneY(
	MechS32 p_normalX,
	MechS32 p_normalY,
	MechS32 p_normalZ,
	MechS32 p_distance,
	MechS32 p_dx,
	MechS32 p_dz
)
{
	/* p_distance is added unsigned (adc edx, 0), and the sum wraps at 64 bits. */
	MechU64 sum = Product(p_normalX, p_dx) + Product(p_normalZ, p_dz) + (MechU32) p_distance;

	return PortableIdiv(PortableS64(sum), p_normalY);
}

// Returns an approximate distance from (p_x, p_y, p_z) to the shape's center, (4 * the largest +
// the others) / 4 of the offsets, or 0x7fffffff outside its bounding sphere.
// Stack-slot permutation: radius, deltaY and deltaZ.
// FUNCTION: MW2 0x10039ccc
MechS32 ApproximateShapeDistance(struct Shape* p_shape, MechS32 p_x, MechS32 p_y, MechS32 p_z)
{
	MechS32 deltaX = Difference(p_shape->m_centerX, p_x);
	MechS32 deltaY = Difference(p_shape->m_centerY, p_y);
	MechS32 deltaZ = Difference(p_shape->m_centerZ, p_z);
	MechS32 radius = p_shape->m_radius;
	MechS32 low = Negate(radius);
	MechS32 largest;
	MechS32 second;
	MechS32 third;
	MechS32 swap;

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

	return PortableS32((((MechU32) largest << 2) + (MechU32) second + (MechU32) third) >> 2);
}

// The normal of the triangle (p_x0, p_y0, p_z0), (p_x1, p_y1, p_z1), (p_x2, p_y2, p_z2): the cross
// product of its edges, scaled to 2.29 fixed point; returns the scale's exponent. A degenerate
// triangle gets the normal (-1, -1, -1) and 0. The products and the scaling are __asm blocks.
// Stack-slot permutation of the locals.
// FUNCTION: MW2 0x10039dda
MechS32 ComputeTriangleNormal(
	MechS32 p_x0,
	MechS32 p_y0,
	MechS32 p_z0,
	MechS32 p_x1,
	MechS32 p_y1,
	MechS32 p_z1,
	MechS32 p_x2,
	MechS32 p_y2,
	MechS32 p_z2,
	MechS32* p_nx,
	MechS32* p_ny,
	MechS32* p_nz
)
{
	MechS64 normal[3];
	MechU32 values[3];
	MechS32 quotients[3];
	MechU32 high = 0;
	MechU32 low = 0;
	MechU32 squares = 0;
	MechS32 divisor;
	MechS32 shift;
	MechS32 scale;
	MechS32 i;

	/* The cross product of the edges, each component a 64-bit difference of products (which
	   can't overflow), and the bits of their magnitudes. */
	normal[0] = (MechS64) Difference(p_y1, p_y0) * Difference(p_z2, p_z1) -
				(MechS64) Difference(p_y2, p_y1) * Difference(p_z1, p_z0);
	normal[1] = (MechS64) Difference(p_z1, p_z0) * Difference(p_x2, p_x1) -
				(MechS64) Difference(p_z2, p_z1) * Difference(p_x1, p_x0);
	normal[2] = (MechS64) Difference(p_x1, p_x0) * Difference(p_y2, p_y1) -
				(MechS64) Difference(p_x2, p_x1) * Difference(p_y1, p_y0);
	for (i = 0; i < 3; i++) {
		MechU64 magnitude = (MechU64) (normal[i] < 0 ? -normal[i] : normal[i]);

		high |= (MechU32) (magnitude >> 32);
		low |= (MechU32) magnitude;
	}

	/* Scaled to 32 bits with the highest bit at 29 or below: shifted right by the magnitude's
	   bits above 29 (the shift count modulo 32, like the shrd's), or left by those missing. */
	if (high) {
		scale = PortableBsr(high) + 3;
		for (i = 0; i < 3; i++) {
			values[i] = (MechU32) ((MechU64) normal[i] >> (scale & 31));
		}

		shift = scale + 29;
	}
	else if (low) {
		shift = PortableBsr(low);
		scale = 29 - shift;
		for (i = 0; i < 3; i++) {
			if (scale < 0) {
				values[i] = (MechU32) ((MechU64) normal[i] >> -scale);
			}
			else {
				values[i] = (MechU32) normal[i] << scale;
			}
		}
	}
	else {
		*p_nx = *p_ny = *p_nz = -1;
		return 0;
	}

	/* Normalized by the square root of the sum of the high words' squares. */
	for (i = 0; i < 3; i++) {
		MechS32 word = PortableS16((MechU16) (values[i] >> 16));

		squares += (MechU32) (word * word);
	}

	divisor = (MechU16) g_sqrtTable[squares >> 20];
	for (i = 0; i < 3; i++) {
		quotients[i] = PortableIdiv((MechS64) PortableS32(values[i]) * 0x2000, divisor);
	}

	*p_nx = Negate(quotients[0]);
	*p_ny = Negate(quotients[1]);
	*p_nz = Negate(quotients[2]);
	return shift;
}

// Returns p_value / (p_a - p_b) in 15.17 fixed point, or 0 if p_a and p_b are equal.
// FUNCTION: MW2 0x1003a05d
MechS32 DivDifference17(MechS32 p_a, MechS32 p_b, MechS32 p_value)
{
	MechS32 divisor = Difference(p_a, p_b);

	if (!divisor) {
		return 0;
	}

	return PortableIdiv((MechS64) p_value * 0x20000, divisor);
}

// Returns how far along p_ray it passes closest to the shape's center, less the radius, or
// 0x7fffffff when it misses the bounding sphere or ends first; 10 when the ray starts inside it.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x1003a096
MechS32 RayShapeDistance(struct Shape* p_shape, Ray* p_ray)
{
	MechS32 radius = p_shape->m_radius;
	MechS32 deltaX;
	MechS32 deltaY;
	MechS32 deltaZ;
	MechU64 excess;
	MechU64 partial;
	MechU64 dot;
	MechS32 t;

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
