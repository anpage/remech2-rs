#include "view.h"

#include "clock.h"
#include "depthsort.h"
#include "eyepoint.h"
#include "faceshade.h"
#include "fixeddot27.h"
#include "fixeddot29.h"
#include "fixedmul.h"
#include "fixedmul29.h"
#include "inradius.h"
#include "muladddiv.h"
#include "muldiv.h"
#include "mulnorm16.h"
#include "mulratio.h"
#include "objectanim.h"
#include "polydraw.h"
#include "shape.h"
#include "shiftdiv.h"
#include "transform.h"
#include "types.h"

// The level of detail: 1 high, 2 low (TOGGLE_LOD_QUALITY); it divides Eyepoint::m_detailScale.
int32_t g_lodQuality = 1;

int32_t g_viewNear;

int32_t g_viewShiftX;

int32_t g_viewShiftY;

int32_t g_viewFar;

int32_t g_viewLeft;

int32_t g_viewCenterX;

int32_t g_viewHalfHeight;

int32_t g_viewHalfWidth;

int32_t g_viewBottom;

int32_t g_viewProjectScaleX;

int32_t g_viewProjectScaleY;

int32_t g_viewRight;

int32_t g_viewTop;

int32_t g_viewLeftScaled;

int32_t g_viewCenterY;

int32_t g_viewBottomScaled;

int32_t g_viewFarPlane;

int32_t g_viewProjX0;

int32_t g_viewProjX1;

int32_t g_viewProjX2;

int32_t g_viewProjY0;

int32_t g_viewProjY1;

int32_t g_viewProjY2;

int32_t g_viewProjZ0;

int32_t g_viewProjZ1;

int32_t g_viewProjZ2;

int32_t g_viewProjectScaleX16;

int32_t g_viewProjectScaleY16;

int32_t g_viewRotX0;

int32_t g_viewRotX1;

int32_t g_viewRotX2;

int32_t g_viewRotY0;

int32_t g_viewRotY1;

int32_t g_viewRotY2;

int32_t g_viewRotZ0;

int32_t g_viewRotZ1;

int32_t g_viewRotZ2;

int32_t g_viewEyeY;

int32_t g_viewEyeX;

int32_t g_viewEyeZ;

int32_t g_viewLightZ;

int32_t g_viewLightX;

int32_t g_viewLightY;

int32_t g_viewTopScaled;

int32_t g_viewNearPlane;

int32_t g_viewRightScaled;

// Makes p_eyepoint the current eyepoint and copies what the renderer uses each frame out of it:
// its rotation (also scaled by the projection factors), position, view rectangle and shading.
void SelectEyepoint(Eyepoint* p_eyepoint)
{
	Eyepoint* eyepoint;

	g_eyepoint = eyepoint = p_eyepoint;
	g_ambientLight = eyepoint->m_ambientLight;
	g_directionalLight = eyepoint->m_directionalLight;
	g_viewProjectScaleX16 = eyepoint->m_projectScaleX16;
	g_viewProjectScaleY16 = eyepoint->m_projectScaleY16;
	g_viewProjectScaleX = eyepoint->m_projectScaleX;
	g_viewProjectScaleY = eyepoint->m_projectScaleY;
	g_viewRotX0 = eyepoint->m_viewMatrix.m_rows[0][0];
	g_viewRotX1 = eyepoint->m_viewMatrix.m_rows[0][1];
	g_viewRotX2 = eyepoint->m_viewMatrix.m_rows[0][2];
	g_viewRotY0 = eyepoint->m_viewMatrix.m_rows[1][0];
	g_viewRotY1 = eyepoint->m_viewMatrix.m_rows[1][1];
	g_viewRotY2 = eyepoint->m_viewMatrix.m_rows[1][2];
	g_viewProjZ0 = g_viewRotZ0 = eyepoint->m_viewMatrix.m_rows[2][0];
	g_viewProjZ1 = g_viewRotZ1 = eyepoint->m_viewMatrix.m_rows[2][1];
	g_viewProjZ2 = g_viewRotZ2 = eyepoint->m_viewMatrix.m_rows[2][2];
	g_viewProjX0 = FixedMul16(g_viewProjectScaleX16, g_viewRotX0);
	g_viewProjX1 = FixedMul16(g_viewProjectScaleX16, g_viewRotX1);
	g_viewProjX2 = FixedMul16(g_viewProjectScaleX16, g_viewRotX2);
	g_viewProjY0 = FixedMul16(g_viewProjectScaleY16, g_viewRotY0);
	g_viewProjY1 = FixedMul16(g_viewProjectScaleY16, g_viewRotY1);
	g_viewProjY2 = FixedMul16(g_viewProjectScaleY16, g_viewRotY2);
	g_viewEyeX = eyepoint->m_viewMatrix.m_rows[3][0];
	g_viewEyeY = eyepoint->m_viewMatrix.m_rows[3][1];
	g_viewEyeZ = eyepoint->m_viewMatrix.m_rows[3][2];
	g_viewLightX = eyepoint->m_lightX;
	g_viewLightY = eyepoint->m_lightY;
	g_viewLightZ = eyepoint->m_lightZ;
	g_viewNearPlane = eyepoint->m_nearPlane;
	g_viewLeft = eyepoint->m_viewLeft;
	g_viewFarPlane = eyepoint->m_farPlane;
	g_viewTop = eyepoint->m_viewTop;
	g_viewRight = eyepoint->m_viewRight;
	g_viewBottom = eyepoint->m_viewBottom;
	g_viewFar = g_viewFarPlane << 2;
	g_viewNear = g_viewNearPlane << 2;
	g_viewTopScaled = g_viewTop << 2;
	g_viewBottomScaled = g_viewBottom << 2;
	g_viewLeftScaled = g_viewLeft << 2;
	g_viewRightScaled = g_viewRight << 2;
	g_viewHalfWidth = eyepoint->m_halfWidth;
	g_viewHalfHeight = eyepoint->m_halfHeight;
	g_viewCenterX = eyepoint->m_centerX;
	g_viewCenterY = eyepoint->m_centerY;
	g_viewShiftX = eyepoint->m_projectShiftX;
	g_viewShiftY = eyepoint->m_projectShiftY;
}

// Sets up the eyepoint's projection from its view rectangle, field of view and pixel aspect.
void UpdateProjection(Eyepoint* p_eyepoint)
{
	int32_t centerX;
	int32_t bottom;
	int32_t c;
	int32_t centerY;
	int32_t right;
	int32_t aspect;
	int32_t left;
	int32_t scaled2;
	int32_t low2;
	int32_t offsetY;
	int16_t shift1;
	int16_t shift2;
	Eyepoint* eyepoint;
	int32_t fovY;
	int32_t d;
	int32_t offsetX;
	int32_t b;
	int32_t halfWidth;
	int32_t top;
	int32_t halfHeight;
	int32_t scaled1;
	int32_t low1;
	int32_t fov;
	int32_t a;

	eyepoint = p_eyepoint;
	fov = eyepoint->m_fovX;
	aspect = eyepoint->m_pixelAspect;
	offsetX = (int16_t) eyepoint->m_offsetX;
	offsetY = (int16_t) eyepoint->m_offsetY;
	top = eyepoint->m_viewTop;
	bottom = eyepoint->m_viewBottom;
	left = eyepoint->m_viewLeft;
	right = eyepoint->m_viewRight;
	eyepoint->m_centerX = centerX = ((right + left + 1) >> 1) + offsetX;
	eyepoint->m_centerY = centerY = ((bottom + top + 1) >> 1) + offsetY;
	eyepoint->m_halfWidth = halfWidth = MECH_MAX((right - left + 1) >> 1, 1);
	eyepoint->m_halfHeight = halfHeight = MECH_MAX((bottom - top + 1) >> 1, 1);
	if (fov > 0x100000) {
		fov = 0x100000;
	}
	if (fov < 0x8000) {
		fov = 0x8000;
	}

	a = MulRatio(fov, halfWidth, offsetX);
	b = MulRatio(fov, halfWidth, -offsetX);
	eyepoint->m_fovY = fovY = MulDiv64(FixedMul16(fov, aspect), halfWidth, halfHeight);
	c = MulRatio(fovY, halfHeight, offsetY);
	d = MulRatio(fovY, halfHeight, -offsetY);
	if (a > 799) {
		a = 799;
	}
	if (b > 799) {
		b = 799;
	}
	if (c > 799) {
		c = 799;
	}
	if (d > 799) {
		d = 799;
	}

	eyepoint->m_frustumScaleX = FixedMul29(fov, g_slopeSines[a]) + (g_slopeCosines[a] >> 13);
	eyepoint->m_frustumScaleY = FixedMul29(fovY, g_slopeSines[c]) + (g_slopeCosines[c] >> 13);
	shift2 = shift1 = 2;
	MulNormalize16(&low1, &scaled1, &shift1, halfWidth, FixedMul16(fov, aspect));
	MulNormalize16(&low2, &scaled2, &shift2, halfWidth, fov);
	eyepoint->m_nearPlane = (low2 >> 17) + 1;
	eyepoint->m_cullDistance = eyepoint->m_farPlane;
	eyepoint->m_projectShiftX = shift2;
	eyepoint->m_projectShiftY = shift1;
	eyepoint->m_projectScaleX16 = scaled2;
	eyepoint->m_projectScaleY16 = scaled1;
	eyepoint->m_projectScaleX = low2;
	eyepoint->m_projectScaleY = low1;
	if (g_lodQuality <= 0) {
		g_lodQuality = 1;
	}

	eyepoint->m_detailScale = low2 / (g_lodQuality * 160);
}

void SetNearPlane(Eyepoint* p_eyepoint, int32_t p_value)
{
	p_eyepoint->m_nearPlane = g_viewNearPlane = p_value;
	g_viewNear = p_value << 2;
}

void SetFarPlane(Eyepoint* p_eyepoint, int32_t p_value)
{
	p_eyepoint->m_farPlane = g_viewFarPlane = p_value;
	if (p_value < 0x1fffffff) {
		g_viewFar = p_value << 2;
		p_eyepoint->m_cullDistance = p_value;
	}
	else {
		g_viewFar = 0x7fffffff;
		p_eyepoint->m_cullDistance = 0x7fffffff;
	}
}

// Builds the eyepoint's view matrix (its rotation transposed, and its position) from its
// position and rotation.
void UpdateViewMatrix(Eyepoint* p_eyepoint)
{
	Matrix matrix;

	BuildMatrix(
		&matrix,
		p_eyepoint->m_pitch,
		p_eyepoint->m_heading,
		p_eyepoint->m_roll,
		p_eyepoint->m_x,
		p_eyepoint->m_y,
		p_eyepoint->m_z
	);
	TransposeRotation(&matrix, &p_eyepoint->m_viewMatrix);
	p_eyepoint->m_viewMatrix.m_rows[3][0] = matrix.m_rows[3][0];
	p_eyepoint->m_viewMatrix.m_rows[3][1] = matrix.m_rows[3][1];
	p_eyepoint->m_viewMatrix.m_rows[3][2] = matrix.m_rows[3][2];
}

void ResetEyepointView(Eyepoint* p_eyepoint)
{
	p_eyepoint->m_offsetX = 0;
	p_eyepoint->m_offsetY = 0;
	UpdateProjection(p_eyepoint);
	UpdateViewMatrix(p_eyepoint);
}

// Sets the eyepoint's view from a transform.
void SetEyepointTransform(Eyepoint* p_eyepoint, Matrix* p_matrix)
{
	TransposeRotation(p_matrix, &p_eyepoint->m_viewMatrix);
	p_eyepoint->m_viewMatrix.m_rows[3][0] = p_matrix->m_rows[3][0];
	p_eyepoint->m_viewMatrix.m_rows[3][1] = p_matrix->m_rows[3][1];
	p_eyepoint->m_viewMatrix.m_rows[3][2] = p_matrix->m_rows[3][2];
}

// Gets the eyepoint's view as a transform.
void GetEyepointTransform(Eyepoint* p_eyepoint, Matrix* p_matrix)
{
	TransposeRotation(&p_eyepoint->m_viewMatrix, p_matrix);
	p_matrix->m_rows[3][0] = p_eyepoint->m_viewMatrix.m_rows[3][0];
	p_matrix->m_rows[3][1] = p_eyepoint->m_viewMatrix.m_rows[3][1];
	p_matrix->m_rows[3][2] = p_eyepoint->m_viewMatrix.m_rows[3][2];
}

// Projects the world point (*p_x, *p_y, *p_z) onto the screen in place (*p_z the depth). A point
// at or behind the near plane is projected mirrored. Returns whether it is in front and on the
// screen.
int32_t ProjectWorldPoint(int32_t* p_x, int32_t* p_y, int32_t* p_z)
{
	int32_t x;
	int32_t y;
	int32_t z;
	int32_t dx;
	int32_t dy;
	int32_t behind;
	int32_t dz;

	x = *p_x;
	y = *p_y;
	z = *p_z;
	dx = x - g_viewEyeX;
	dy = y - g_viewEyeY;
	dz = z - g_viewEyeZ;
	x = FixedDot27(dx, g_viewProjX0, dy, g_viewProjX1, dz, g_viewProjX2);
	y = FixedDot27(dx, g_viewProjY0, dy, g_viewProjY1, dz, g_viewProjY2);
	z = FixedDot27(dx, g_viewProjZ0, dy, g_viewProjZ1, dz, g_viewProjZ2);
	*p_z = z;
	if (z <= g_viewNear) {
		behind = true;
		if (z < 0) {
			z = -z;
		}
		else if (z == 0) {
			z = 1;
		}
	}
	else {
		behind = false;
	}

	*p_x = ProjectCoordinate(x, z, g_viewShiftX, g_viewCenterX);
	*p_y = g_viewBottom - g_viewTop - ProjectCoordinate(y, z, g_viewShiftY, g_viewCenterY);
	if (behind) {
		return 0;
	}

	return *p_x >= g_viewLeft && *p_x <= g_viewRight && *p_y >= g_viewTop && *p_y <= g_viewBottom;
}

// The scene's shape filter (RenderSettings::m_shapeFilter): culls a shape against the view frustum,
// like CullMapViewShape the map view's. 1: a shape of kind 0xa0 with g_inCockpitView, 5: out of
// range or past the far plane, 4: in front of the near plane, 6 and 7: outside the side planes.
int32_t CullSceneShape(Shape* p_shape)
{
	int32_t y;
	int32_t z;
	int32_t radius;
	int32_t limit;
	int32_t dx;
	int32_t side;
	int32_t dy;
	int32_t height;
	int32_t fov;
	int32_t dz;
	int32_t depth;
	int32_t x;

	if (g_inCockpitView && (p_shape->m_kind & 0xf0) == 0xa0) {
		return 1;
	}

	x = p_shape->m_centerX;
	y = p_shape->m_centerY;
	z = p_shape->m_centerZ;
	radius = p_shape->m_radius;
	dx = x - g_viewEyeX;
	dy = y - g_viewEyeY;
	dz = z - g_viewEyeZ;
	if (!IsWithinRadius(dx, dy, dz, g_eyepoint->m_cullDistance + radius)) {
		return 5;
	}

	depth = g_queueDepth = FixedDot29(dx, g_viewRotZ0, dy, g_viewRotZ1, dz, g_viewRotZ2);
	if (radius + depth < g_viewNearPlane) {
		return 4;
	}

	if (depth - radius > g_viewFarPlane) {
		return 5;
	}

	side = FixedDot29(dx, g_viewRotX0, dy, g_viewRotX1, dz, g_viewRotX2);
	fov = g_eyepoint->m_fovX;
	if (side > 0) {
		limit = MulAddDiv(fov, side, -depth, g_eyepoint->m_frustumScaleX);
	}
	else {
		limit = MulAddDiv(fov, -side, -depth, g_eyepoint->m_frustumScaleX);
	}

	if (limit > radius) {
		return 6;
	}

	height = FixedDot29(dx, g_viewRotY0, dy, g_viewRotY1, dz, g_viewRotY2);
	fov = g_eyepoint->m_fovY;
	if (height > 0) {
		limit = MulAddDiv(fov, height, -depth, g_eyepoint->m_frustumScaleY);
	}
	else {
		limit = MulAddDiv(fov, -height, -depth, g_eyepoint->m_frustumScaleY);
	}

	if (limit > radius) {
		return 7;
	}

	return 0;
}

// Culls a shape against the view frustum like CullSceneShape, without its range test. 1 for a
// hidden shape (bit 0x1000).
int32_t CullShapeToFrustum(Shape* p_shape)
{
	int32_t y;
	int32_t z;
	int32_t radius;
	int32_t limit;
	int32_t dx;
	int32_t side;
	int32_t dy;
	int32_t height;
	int32_t fov;
	int32_t dz;
	int32_t depth;
	int32_t x;

	x = p_shape->m_centerX;
	y = p_shape->m_centerY;
	z = p_shape->m_centerZ;
	radius = p_shape->m_radius;
	dx = x - g_viewEyeX;
	dy = y - g_viewEyeY;
	dz = z - g_viewEyeZ;
	if (p_shape->m_flags & 0x1000) {
		return 1;
	}

	depth = g_queueDepth = FixedDot29(dx, g_viewRotZ0, dy, g_viewRotZ1, dz, g_viewRotZ2);
	if (radius + depth < g_viewNearPlane) {
		return 4;
	}

	side = FixedDot29(dx, g_viewRotX0, dy, g_viewRotX1, dz, g_viewRotX2);
	fov = g_eyepoint->m_fovX;
	if (side > 0) {
		limit = MulAddDiv(fov, side, -depth, g_eyepoint->m_frustumScaleX);
	}
	else {
		limit = MulAddDiv(fov, -side, -depth, g_eyepoint->m_frustumScaleX);
	}

	if (limit > radius) {
		return 6;
	}

	height = FixedDot29(dx, g_viewRotY0, dy, g_viewRotY1, dz, g_viewRotY2);
	fov = g_eyepoint->m_fovY;
	if (height > 0) {
		limit = MulAddDiv(fov, height, -depth, g_eyepoint->m_frustumScaleY);
	}
	else {
		limit = MulAddDiv(fov, -height, -depth, g_eyepoint->m_frustumScaleY);
	}

	if (limit > radius) {
		return 7;
	}

	return 0;
}

int32_t CullHiddenShape(Shape* p_shape)
{
	if (p_shape->m_flags & 0x1000) {
		return true;
	}

	return false;
}

int32_t IsLodQualityHigh(uint32_t p_unk0x00)
{
	return g_lodQuality == 1;
}

void SetLodQualityHigh(uint32_t p_unk0x00, int32_t p_enable)
{
	if (p_enable) {
		g_lodQuality = 1;
	}
	else {
		g_lodQuality = 2;
	}
}
