#include "ray.h"

#include "approxlen.h"
#include "decomp.h"
#include "fixeddiv.h"
#include "fixedmul.h"
#include "muldiv.h"
#include "types.h"

#include <math.h>

DECOMP_SIZE_ASSERT(Ray, 0x38)

// FUNCTION: MW2 0x10002e20
void BuildRayFromSegment(Ray* p_ray, MechS32 p_x0, MechS32 p_y0, MechS32 p_z0, MechS32 p_x1, MechS32 p_y1, MechS32 p_z1)
{
	p_ray->m_x0 = p_x0;
	p_ray->m_y0 = p_y0;
	p_ray->m_z0 = p_z0;
	p_ray->m_x1 = p_x1;
	p_ray->m_y1 = p_y1;
	p_ray->m_z1 = p_z1;
	p_ray->m_dx = p_x1 - p_x0;
	p_ray->m_dy = p_y1 - p_y0;
	p_ray->m_dz = p_z1 - p_z0;
	p_ray->m_dirX = p_ray->m_dirY = p_ray->m_dirZ = p_ray->m_length = 0;
	p_ray->m_state = c_rayNone;
}

// FUNCTION: MW2 0x10002ebc
void BuildRayFromDirection(
	Ray* p_ray,
	MechS32 p_x0,
	MechS32 p_y0,
	MechS32 p_z0,
	MechS32 p_dirX,
	MechS32 p_dirY,
	MechS32 p_dirZ,
	MechS32 p_length
)
{
	p_ray->m_x0 = p_x0;
	p_ray->m_y0 = p_y0;
	p_ray->m_z0 = p_z0;
	p_ray->m_dirX = p_dirX;
	p_ray->m_dirY = p_dirY;
	p_ray->m_dirZ = p_dirZ;
	p_ray->m_dx = FixedMul16(p_length, p_dirX);
	p_ray->m_dy = FixedMul16(p_length, p_dirY);
	p_ray->m_dz = FixedMul16(p_length, p_dirZ);
	p_ray->m_x1 = p_ray->m_dx + p_x0;
	p_ray->m_y1 = p_ray->m_dy + p_y0;
	p_ray->m_z1 = p_ray->m_dz + p_z0;
	p_ray->m_length = p_length;
	p_ray->m_state = c_rayFixed;
}

// FUNCTION: MW2 0x10002f7e
MechS32 GetRayLength(Ray* p_ray)
{
	if (p_ray->m_state == c_rayNone) {
		BuildRayFixed(p_ray);
	}

	return p_ray->m_length;
}

// FUNCTION: MW2 0x10002fad
void BuildRayFixed(Ray* p_ray)
{
	MechS32 length;

	if (p_ray->m_state == c_rayNone) {
		p_ray->m_length = ApproximateVectorLength(p_ray->m_dx, p_ray->m_dy, p_ray->m_dz);
		length = p_ray->m_length;
		if (length > 0) {
			p_ray->m_dirX = FixedDiv16(p_ray->m_dx, length);
			p_ray->m_dirY = FixedDiv16(p_ray->m_dy, length);
			p_ray->m_dirZ = FixedDiv16(p_ray->m_dz, length);
			p_ray->m_state = c_rayFixed;
		}
	}
}

// FUNCTION: MW2 0x10003053
void BuildRayFloat(Ray* p_ray)
{
	MechDouble dx;
	MechDouble dy;
	MechDouble dz;
	MechDouble length;

	if (p_ray->m_state == c_rayFloat) {
		return;
	}

	dx = p_ray->m_x1 - p_ray->m_x0;
	dy = p_ray->m_y1 - p_ray->m_y0;
	dz = p_ray->m_z1 - p_ray->m_z0;
	dx /= length = sqrt(dx * dx + dy * dy + dz * dz);
	dy /= length;
	dz /= length;
	p_ray->m_dirX = (MechS32) (dx * 65536.0);
	p_ray->m_dirY = (MechS32) (dy * 65536.0);
	p_ray->m_dirZ = (MechS32) (dz * 65536.0);
	p_ray->m_length = (MechS32) (length + 0.5);
	p_ray->m_state = c_rayFloat;
}

// FUNCTION: MW2 0x1000313e
void SetRayLength(Ray* p_ray, MechS32 p_length)
{
	if (p_ray->m_state == c_rayNone) {
		BuildRayFixed(p_ray);
	}

	if (p_ray->m_state != c_rayNone) {
		p_ray->m_x1 = p_ray->m_x0 + FixedMul16(p_length, p_ray->m_dirX);
		p_ray->m_y1 = p_ray->m_y0 + FixedMul16(p_length, p_ray->m_dirY);
		p_ray->m_z1 = p_ray->m_z0 + FixedMul16(p_length, p_ray->m_dirZ);
		p_ray->m_dx = p_ray->m_x1 - p_ray->m_x0;
		p_ray->m_dy = p_ray->m_y1 - p_ray->m_y0;
		p_ray->m_dz = p_ray->m_z1 - p_ray->m_z0;
		p_ray->m_length = p_length;
	}
}

// FUNCTION: MW2 0x1000320f
void AdvanceRayStart(Ray* p_ray, MechS32 p_distance)
{
	MechS32 state;

	state = p_ray->m_state;
	if (state == c_rayNone) {
		BuildRayFixed(p_ray);
	}

	if (p_ray->m_length > 0) {
		p_ray->m_x0 += FixedMul16(p_distance, p_ray->m_dirX);
		p_ray->m_y0 += FixedMul16(p_distance, p_ray->m_dirY);
		p_ray->m_z0 += FixedMul16(p_distance, p_ray->m_dirZ);
		p_ray->m_dx = p_ray->m_x1 - p_ray->m_x0;
		p_ray->m_dy = p_ray->m_y1 - p_ray->m_y0;
		p_ray->m_dz = p_ray->m_z1 - p_ray->m_z0;
		p_ray->m_state = c_rayNone;
		if (state == c_rayFloat) {
			BuildRayFloat(p_ray);
		}
		else {
			BuildRayFixed(p_ray);
		}
	}
}

// FUNCTION: MW2 0x100032f9
void SetRayEnd(Ray* p_ray, MechS32 p_x1, MechS32 p_y1, MechS32 p_z1)
{
	MechS32 state;

	p_ray->m_x1 = p_x1;
	p_ray->m_y1 = p_y1;
	p_ray->m_z1 = p_z1;
	p_ray->m_dx = p_x1 - p_ray->m_x0;
	p_ray->m_dy = p_y1 - p_ray->m_y0;
	p_ray->m_dz = p_z1 - p_ray->m_z0;
	state = p_ray->m_state;
	p_ray->m_state = c_rayNone;
	switch (state) {
	case c_rayFixed:
		BuildRayFixed(p_ray);
		break;
	case c_rayFloat:
		BuildRayFloat(p_ray);
		break;
	default:
		p_ray->m_dirX = p_ray->m_dirY = p_ray->m_dirZ = p_ray->m_length = 0;
		break;
	}
}

// FUNCTION: MW2 0x100033df
void ClipRayToGround(Ray* p_ray, MechS32 p_y)
{
	MechS32 length;

	if (p_ray->m_dy) {
		length = MulDiv64(p_ray->m_length, p_y - p_ray->m_y0, p_ray->m_dy);
		SetRayLength(p_ray, length);
	}
}

// FUNCTION: MW2 0x1000342d
void CopyRay(Ray* p_dst, Ray* p_src)
{
	*p_dst = *p_src;
}

// Matches except for the stack slots of toMax and quotient (a consistent permutation).
// FUNCTION: MW2 0x10003445
MechS32 ClipRaySlab(MechS32 p_origin, MechS32 p_delta, MechS32 p_min, MechS32 p_max, MechS32* p_tMin, MechS32* p_tMax)
{
	MechS32 toMax;
	MechS32 t0;
	MechS32 t1;
	MechS32 toMin;
	MechS32 overflow;
	MechS32 quotient;

	overflow = FALSE;
	if (p_delta == 0) {
		overflow = TRUE;
	}
	else {
		toMin = p_min - p_origin;
		toMax = p_max - p_origin;
		quotient = toMin / p_delta;
		if (quotient > 0x7fff || quotient < -0x8000) {
			overflow = TRUE;
		}
		else {
			quotient = toMax / p_delta;
			if (quotient > 0x7fff || quotient < -0x8000) {
				overflow = TRUE;
			}
		}
	}

	if (overflow) {
		if (p_origin < p_min || p_origin > p_max) {
			return 1;
		}

		t0 = 0x80000001;
		t1 = 0x7fffffff;
	}
	else {
		t0 = FixedDiv16(toMin, p_delta);
		t1 = FixedDiv16(toMax, p_delta);
	}

	if (t1 < t0) {
		*p_tMin = t1;
		*p_tMax = t0;
	}
	else {
		*p_tMin = t0;
		*p_tMax = t1;
	}

	return 0;
}
