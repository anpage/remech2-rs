#include "ray.h"

#include "approxlen.h"
#include "fixeddiv.h"
#include "fixedmul.h"
#include "muldiv.h"
#include "types.h"

#include <math.h>

void BuildRayFromSegment(Ray* p_ray, int32_t p_x0, int32_t p_y0, int32_t p_z0, int32_t p_x1, int32_t p_y1, int32_t p_z1)
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

void BuildRayFromDirection(
	Ray* p_ray,
	int32_t p_x0,
	int32_t p_y0,
	int32_t p_z0,
	int32_t p_dirX,
	int32_t p_dirY,
	int32_t p_dirZ,
	int32_t p_length
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

int32_t GetRayLength(Ray* p_ray)
{
	if (p_ray->m_state == c_rayNone) {
		BuildRayFixed(p_ray);
	}

	return p_ray->m_length;
}

void BuildRayFixed(Ray* p_ray)
{
	int32_t length;

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

void BuildRayFloat(Ray* p_ray)
{
	double dx;
	double dy;
	double dz;
	double length;

	if (p_ray->m_state == c_rayFloat) {
		return;
	}

	dx = p_ray->m_x1 - p_ray->m_x0;
	dy = p_ray->m_y1 - p_ray->m_y0;
	dz = p_ray->m_z1 - p_ray->m_z0;
	dx /= length = sqrt(dx * dx + dy * dy + dz * dz);
	dy /= length;
	dz /= length;
	p_ray->m_dirX = (int32_t) (dx * 65536.0);
	p_ray->m_dirY = (int32_t) (dy * 65536.0);
	p_ray->m_dirZ = (int32_t) (dz * 65536.0);
	p_ray->m_length = (int32_t) (length + 0.5);
	p_ray->m_state = c_rayFloat;
}

void SetRayLength(Ray* p_ray, int32_t p_length)
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

void AdvanceRayStart(Ray* p_ray, int32_t p_distance)
{
	int32_t state;

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

void SetRayEnd(Ray* p_ray, int32_t p_x1, int32_t p_y1, int32_t p_z1)
{
	int32_t state;

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

void ClipRayToGround(Ray* p_ray, int32_t p_y)
{
	int32_t length;

	if (p_ray->m_dy) {
		length = MulDiv64(p_ray->m_length, p_y - p_ray->m_y0, p_ray->m_dy);
		SetRayLength(p_ray, length);
	}
}

void CopyRay(Ray* p_dst, Ray* p_src)
{
	*p_dst = *p_src;
}

int32_t ClipRaySlab(int32_t p_origin, int32_t p_delta, int32_t p_min, int32_t p_max, int32_t* p_tMin, int32_t* p_tMax)
{
	int32_t toMax;
	int32_t t0;
	int32_t t1;
	int32_t toMin;
	int32_t overflow;
	int32_t quotient;

	overflow = false;
	if (p_delta == 0) {
		overflow = true;
	}
	else {
		toMin = p_min - p_origin;
		toMax = p_max - p_origin;
		quotient = toMin / p_delta;
		if (quotient > 0x7fff || quotient < -0x8000) {
			overflow = true;
		}
		else {
			quotient = toMax / p_delta;
			if (quotient > 0x7fff || quotient < -0x8000) {
				overflow = true;
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
