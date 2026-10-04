/* Hand-written assembly: TransformShapeCenter, SolvePlaneY, ApproximateShapeDistance, ComputeTriangleNormal,
   DivDifference17 and RayShapeDistance are C functions with __asm bodies, and TransformModel has an __asm block. Their
   portable C (PORTABLE_C) is tested against the assembly by tests/asmequiv: it replaces
   TransformShapeCenter's __asm block, and the others' whole bodies, whose C wraps where standard C
   overflows. */
#include "shapegeom.h"

#include "clock.h"
#include "compat.h"
#include "decomp.h"
#include "face.h"
#include "portable.h"
#include "ray.h"
#include "rendersettings.h"
#include "shape.h"
#include "transform.h"
#include "types.h"
#include "vertex.h"

#pragma warning(disable : 4102) /* a label only an __asm block jumps to */

/* The __asm blocks of TransformModel, ApproximateShapeDistance, ComputeTriangleNormal and RayShapeDistance jump to C
   labels, which newer compilers reject: their reference build (REFERENCE_ASM) compiles those functions' portable C too.
 */
#if defined(PORTABLE_C) || !defined(_MSC_VER) || _MSC_VER >= 1100
#define PORTABLE_C_LABELS

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
#endif

// Transforms p_model's vertices (their positions into m_worldX-m_worldZ) and face normals by
// p_matrix. The products are an __asm block.
// Stack-slot permutation of the locals.
// FUNCTION: MW2 0x10039a30
void TransformModel(Model* p_model, Matrix* p_matrix)
{
#ifdef PORTABLE_C_LABELS
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
#else
	Face* faces;
	MechS16 vertexCount;
	MechS32 vertexSize;
	Vertex* vertices;
	MechS16 faceCount;
	MechS32 faceSize;

	vertices = (Vertex*) (p_model + 1);
	faces = (Face*) ((MechU8*) p_model + p_model->m_faceOffset);
	vertexSize = sizeof(Vertex);
	faceSize = sizeof(Face);
	vertexCount = p_model->m_vertexCount;
	faceCount = p_model->m_faceCount;
	__asm {
		mov esi, p_matrix
		mov edi, vertices
	jmp_10039a78:
		mov eax, dword ptr [esi]
		imul dword ptr [edi]
		mov ecx, edx
		mov ebx, eax
		mov eax, dword ptr [esi + 4]
		imul dword ptr [edi + 4]
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [esi + 8]
		imul dword ptr [edi + 8]
		add eax, ebx
		adc edx, ecx
		shrd eax, edx, 0x1d
		adc eax, dword ptr [esi + 0x24]
		mov dword ptr [edi + 0xc], eax
		mov eax, dword ptr [esi + 0xc]
		imul dword ptr [edi]
		mov ecx, edx
		mov ebx, eax
		mov eax, dword ptr [esi + 0x10]
		imul dword ptr [edi + 4]
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [esi + 0x14]
		imul dword ptr [edi + 8]
		add eax, ebx
		adc edx, ecx
		shrd eax, edx, 0x1d
		adc eax, dword ptr [esi + 0x28]
		mov dword ptr [edi + 0x10], eax
		mov eax, dword ptr [esi + 0x18]
		imul dword ptr [edi]
		mov ecx, edx
		mov ebx, eax
		mov eax, dword ptr [esi + 0x1c]
		imul dword ptr [edi + 4]
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [esi + 0x20]
		imul dword ptr [edi + 8]
		add eax, ebx
		adc edx, ecx
		shrd eax, edx, 0x1d
		adc eax, dword ptr [esi + 0x2c]
		mov dword ptr [edi + 0x14], eax
		add edi, vertexSize
		dec vertexCount
		je jmp_10039afe
		_emit 0xe9 /* jmp jmp_10039a78 */
		_emit 0x7a
		_emit 0xff
		_emit 0xff
		_emit 0xff
	jmp_10039afe:
		mov edi, faces
	jmp_10039b01:
		mov eax, dword ptr [esi]
		imul dword ptr [edi + 8]
		mov ecx, edx
		mov ebx, eax
		mov eax, dword ptr [esi + 4]
		imul dword ptr [edi + 0xc]
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [esi + 8]
		imul dword ptr [edi + 0x10]
		add eax, ebx
		adc edx, ecx
		shrd eax, edx, 0x1d
		adc eax, 0
		mov dword ptr [edi + 0x14], eax
		mov eax, dword ptr [esi + 0xc]
		imul dword ptr [edi + 8]
		mov ecx, edx
		mov ebx, eax
		mov eax, dword ptr [esi + 0x10]
		imul dword ptr [edi + 0xc]
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [esi + 0x14]
		imul dword ptr [edi + 0x10]
		add eax, ebx
		adc edx, ecx
		shrd eax, edx, 0x1d
		adc eax, 0
		mov dword ptr [edi + 0x18], eax
		mov eax, dword ptr [esi + 0x18]
		imul dword ptr [edi + 8]
		mov ecx, edx
		mov ebx, eax
		mov eax, dword ptr [esi + 0x1c]
		imul dword ptr [edi + 0xc]
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [esi + 0x20]
		imul dword ptr [edi + 0x10]
		add eax, ebx
		adc edx, ecx
		shrd eax, edx, 0x1d
		adc eax, 0
		mov dword ptr [edi + 0x1c], eax
		add edi, faceSize
		dec faceCount
		je jmp_10039b8a
		_emit 0xe9 /* jmp jmp_10039b01 */
		_emit 0x77
		_emit 0xff
		_emit 0xff
		_emit 0xff
	}

	jmp_10039b8a : return;
#endif
}

// Transforms the shape's position (m_modelCenterX-Z) by p_matrix into m_centerX-Z, and bumps its
// transform count (m_transformCount). The products are an __asm block.
// FUNCTION: MW2 0x10039b94
void TransformShapeCenter(struct Shape* p_shape, Matrix* p_matrix)
{
	p_shape->m_flags &= ~0x200;
#ifdef PORTABLE_C
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
#else
	__asm {
		mov esi, p_matrix
		mov edi, p_shape
		mov eax, dword ptr [esi]
		imul dword ptr [edi + 0x28]
		mov ecx, edx
		mov ebx, eax
		mov eax, dword ptr [esi + 0x4]
		imul dword ptr [edi + 0x2c]
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [esi + 0x8]
		imul dword ptr [edi + 0x30]
		add eax, ebx
		adc edx, ecx
		shrd eax, edx, 29
		adc eax, dword ptr [esi + 0x24]
		mov dword ptr [edi + 0x34], eax
		mov eax, dword ptr [esi + 0xc]
		imul dword ptr [edi + 0x28]
		mov ecx, edx
		mov ebx, eax
		mov eax, dword ptr [esi + 0x10]
		imul dword ptr [edi + 0x2c]
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [esi + 0x14]
		imul dword ptr [edi + 0x30]
		add eax, ebx
		adc edx, ecx
		shrd eax, edx, 29
		adc eax, dword ptr [esi + 0x28]
		mov dword ptr [edi + 0x38], eax
		mov eax, dword ptr [esi + 0x18]
		imul dword ptr [edi + 0x28]
		mov ecx, edx
		mov ebx, eax
		mov eax, dword ptr [esi + 0x1c]
		imul dword ptr [edi + 0x2c]
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [esi + 0x20]
		imul dword ptr [edi + 0x30]
		add eax, ebx
		adc edx, ecx
		shrd eax, edx, 29
		adc eax, dword ptr [esi + 0x2c]
		mov dword ptr [edi + 0x3c], eax
	}
#endif

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
#ifdef PORTABLE_C
	/* p_distance is added unsigned (adc edx, 0), and the sum wraps at 64 bits. */
	MechU64 sum = Product(p_normalX, p_dx) + Product(p_normalZ, p_dz) + (MechU32) p_distance;

	return PortableIdiv(PortableS64(sum), p_normalY);
#else
	MechS32 result;

	__asm {
		mov eax, p_normalX
		imul p_dx
		mov esi, eax
		mov edi, edx
		mov eax, p_normalZ
		imul p_dz
		add eax, esi
		adc edx, edi
		add eax, p_distance
		adc edx, 0
		idiv p_normalY
		mov result, eax
	}

	return result;
#endif
}

// Returns an approximate distance from (p_x, p_y, p_z) to the shape's center, (4 * the largest +
// the others) / 4 of the offsets, or 0x7fffffff outside its bounding sphere.
// Stack-slot permutation: radius, deltaY and deltaZ.
// FUNCTION: MW2 0x10039ccc
MechS32 ApproximateShapeDistance(struct Shape* p_shape, MechS32 p_x, MechS32 p_y, MechS32 p_z)
{
#ifdef PORTABLE_C_LABELS
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
#else
	MechS32 result;
	MechS32 radius;
	MechS32 deltaX;
	MechS32 deltaY;
	MechS32 deltaZ;

	deltaX = p_shape->m_centerX - p_x;
	deltaY = p_shape->m_centerY - p_y;
	deltaZ = p_shape->m_centerZ - p_z;
	radius = p_shape->m_radius;
	result = 0;
	__asm {
		mov ebx, radius
		mov ecx, ebx
		neg ecx
		mov eax, deltaX
		cmp eax, ebx
		jg jmp_10039d70
		cmp eax, ecx
		jl jmp_10039d70
		mov eax, deltaY
		cmp eax, ebx
		jg jmp_10039d70
		cmp eax, ecx
		jl jmp_10039d70
		mov eax, deltaZ
		cmp eax, ebx
		jg jmp_10039d70
		cmp eax, ecx
		jl jmp_10039d70
		imul eax
		mov ebx, eax
		mov ecx, edx
		mov eax, deltaX
		imul eax
		add ebx, eax
		adc ecx, edx
		mov eax, deltaY
		imul eax
		add ebx, eax
		adc ecx, edx
		mov eax, radius
		imul eax
		sub eax, ebx
		sbb edx, ecx
		jae jmp_10039d7a
jmp_10039d70:
		mov eax, 0x7fffffff
		jmp done
jmp_10039d7a:
		mov eax, deltaX
		cmp eax, 0
		jge jmp_10039d88
		neg eax
jmp_10039d88:
		mov ebx, deltaY
		cmp ebx, 0
		jge jmp_10039d96
		neg ebx
jmp_10039d96:
		cmp eax, ebx
		jge jmp_10039da4
		mov edx, eax
		mov eax, ebx
		mov ebx, edx
jmp_10039da4:
		mov ecx, deltaZ
		cmp ecx, 0
		jge jmp_10039db2
		neg ecx
jmp_10039db2:
		cmp eax, ecx
		jge jmp_10039dc0
		mov edx, eax
		mov eax, ecx
		mov ecx, edx
jmp_10039dc0:
		shl eax, 2
		add eax, ebx
		add eax, ecx
		shr eax, 2
		mov result, eax
	}

	return result;
done:;
#endif
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
#ifdef PORTABLE_C_LABELS
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
#else
	MechS32 nyAbs;
	MechS32 nxAbs;
	MechS32 nzAbs;
	MechS32 nyHigh;
	MechS32 nx;
	MechS32 nzHigh;
	MechS32 ny;
	MechS16 shift;
	MechS32 nz;
	MechS32 nyAbsHigh;
	MechS32 nxAbsHigh;
	MechS32 nzAbsHigh;
	MechS32 nxHigh;

	__asm {
		mov eax, p_y1
		sub eax, p_y0
		mov ecx, p_z2
		sub ecx, p_z1
		imul ecx
		mov edi, edx
		mov esi, eax
		mov eax, p_y2
		sub eax, p_y1
		mov ecx, p_z1
		sub ecx, p_z0
		imul ecx
		sub esi, eax
		sbb edi, edx
		mov nxHigh, edi
		mov nx, esi
		jge jmp_10039e1d
		not edi
		not esi
		add esi, 1
		adc edi, 0
jmp_10039e1d:
		mov nxAbsHigh, edi
		mov nxAbs, esi
		mov eax, p_z1
		sub eax, p_z0
		mov ecx, p_x2
		sub ecx, p_x1
		imul ecx
		mov edi, edx
		mov esi, eax
		mov eax, p_z2
		sub eax, p_z1
		mov ecx, p_x1
		sub ecx, p_x0
		imul ecx
		sub esi, eax
		sbb edi, edx
		mov nyHigh, edi
		mov ny, esi
		jge jmp_10039e5d
		not edi
		not esi
		add esi, 1
		adc edi, 0
jmp_10039e5d:
		mov nyAbsHigh, edi
		mov nyAbs, esi
		mov eax, p_x1
		sub eax, p_x0
		mov ecx, p_y2
		sub ecx, p_y1
		imul ecx
		mov edi, edx
		mov esi, eax
		mov eax, p_x2
		sub eax, p_x1
		mov ecx, p_y1
		sub ecx, p_y0
		imul ecx
		sub esi, eax
		sbb edi, edx
		mov nzHigh, edi
		mov nz, esi
		jge jmp_10039e9d
		not edi
		not esi
		add esi, 1
		adc edi, 0
jmp_10039e9d:
		mov nzAbsHigh, edi
		mov nzAbs, esi
		or esi, nyAbs
		or esi, nxAbs
		or edi, nyAbsHigh
		or edi, nxAbsHigh
		je jmp_10039f0e
		xor ax, ax
		test edi, 0xffff0000
		je jmp_10039ecb
		add ax, 0x10
		shr edi, 0x10
jmp_10039ecb:
		test edi, 0xff00
		je jmp_10039ede
		add ax, 8
		shr edi, 8
jmp_10039ede:
		shl edi, 8
		bsr cx, di
		sub cx, 5
		add cx, ax
		mov eax, nxHigh
		shrd nx, eax, cl
		mov eax, nyHigh
		shrd ny, eax, cl
		mov eax, nzHigh
		shrd nz, eax, cl
		add cx, 0x1d
		mov shift, cx
		jmp done
jmp_10039f0e:
		or esi, esi
		je zero
		mov ax, 0x18
		test esi, 0xffff0000
		je jmp_10039f2d
		sub ax, 0x10
		shr esi, 0x10
jmp_10039f2d:
		test esi, 0xff00
		je jmp_10039f40
		sub ax, 8
		shr esi, 8
jmp_10039f40:
		shl esi, 8
		bsr cx, si
		neg cx
		add cx, 0xd
		add cx, ax
		je jmp_10039f75
		jg jmp_10039f82
		neg cx
		mov eax, nxHigh
		shrd nx, eax, cl
		mov eax, nyHigh
		shrd ny, eax, cl
		mov eax, nzHigh
		shrd nz, eax, cl
jmp_10039f75:
		add cx, 0x1d
		mov shift, cx
		jmp done
jmp_10039f82:
		shl nx, cl
		shl ny, cl
		shl nz, cl
		mov ax, 0x1d
		sub ax, cx
		mov shift, ax
done:
	}
	goto normalize;

zero:
	*p_nx = *p_ny = *p_nz = -1;
	return 0;

normalize:
	__asm {
		mov eax, nx
		sar eax, 0x10
		imul ax
		xor ebx, ebx
		mov bx, dx
		mov cx, ax
		mov eax, ny
		sar eax, 0x10
		imul ax
		add cx, ax
		adc bx, dx
		mov eax, nz
		sar eax, 0x10
		imul ax
		add cx, ax
		adc bx, dx
		shr bx, 4
		shl bx, 1
		add ebx, dword ptr [g_sqrtTable]
		mov cx, word ptr [ebx]
		movzx ecx, cx
		mov eax, nx
		cdq
		shld edx, eax, 0xd
		shl eax, 0xd
		idiv ecx
		mov nx, eax
		mov eax, ny
		cdq
		shld edx, eax, 0xd
		shl eax, 0xd
		idiv ecx
		mov ny, eax
		mov eax, nz
		cdq
		shld edx, eax, 0xd
		shl eax, 0xd
		idiv ecx
		mov nz, eax
	}

	*p_nx = -nx;
	*p_ny = -ny;
	*p_nz = -nz;
	return shift;
#endif
}

// Returns p_value / (p_a - p_b) in 15.17 fixed point, or 0 if p_a and p_b are equal.
// FUNCTION: MW2 0x1003a05d
MechS32 DivDifference17(MechS32 p_a, MechS32 p_b, MechS32 p_value)
{
#ifdef PORTABLE_C
	MechS32 divisor = Difference(p_a, p_b);

	if (!divisor) {
		return 0;
	}

	return PortableIdiv((MechS64) p_value * 0x20000, divisor);
#else
	MechS32 result;

	result = 0;
	__asm {
		mov ebx, p_a
		sub ebx, p_b
		jz done
		mov eax, p_value
		cdq
		shld edx, eax, 17
		shl eax, 17
		idiv ebx
		mov result, eax
done:
	}

	return result;
#endif
}

// Returns how far along p_ray it passes closest to the shape's center, less the radius, or
// 0x7fffffff when it misses the bounding sphere or ends first; 10 when the ray starts inside it.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x1003a096
MechS32 RayShapeDistance(struct Shape* p_shape, Ray* p_ray)
{
#ifdef PORTABLE_C_LABELS
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
#else
	MechS32 deltaX;
	MechS32 radius;
	MechS32 deltaY;
	MechS32 excess;
	MechS32 deltaZ;
	MechS32 t;

	radius = p_shape->m_radius;
	if (radius <= 0) {
		return 0x7fffffff;
	}

	deltaX = p_shape->m_centerX - p_ray->m_x0;
	deltaY = p_shape->m_centerY - p_ray->m_y0;
	deltaZ = p_shape->m_centerZ - p_ray->m_z0;
	__asm {
		mov eax, deltaX
		imul eax
		mov ebx, eax
		mov ecx, edx
		mov eax, deltaY
		imul eax
		add ebx, eax
		adc ecx, edx
		mov eax, deltaZ
		imul eax
		add ebx, eax
		adc ecx, edx
		mov eax, radius
		imul eax
		sub ebx, eax
		sbb ecx, edx
		jae jmp_1003a11c
		mov eax, 10
		jmp done
jmp_1003a11c:
		mov excess, ebx
		mov ebx, p_ray
		mov eax, deltaX
		imul dword ptr [ebx + 0x18]
		mov edi, eax
		mov esi, edx
		mov eax, deltaY
		imul dword ptr [ebx + 0x1c]
		add edi, eax
		adc esi, edx
		mov eax, deltaZ
		imul dword ptr [ebx + 0x20]
		add edi, eax
		adc esi, edx
		jae jmp_1003a150
		mov eax, 0x7fffffff
		jmp done
jmp_1003a150:
		mov edx, esi
		mov eax, edi
		idiv dword ptr [ebx + 0x30]
		mov t, eax
		mov ebx, excess
		imul eax
		sub eax, ebx
		sbb edx, ecx
		jae jmp_1003a173
		mov eax, 0x7fffffff
		jmp done
jmp_1003a173:
	}

	if ((t -= radius) < 0)
	{
		t = 0;
	}

	return t >= GetRayLength(p_ray) ? 0x7fffffff : t;
done:;
#endif
}
