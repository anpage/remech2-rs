#ifndef VIEW_H
#define VIEW_H

#include "eyepoint.h"
#include "types.h"

struct Shape;

// The functions and globals of view.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_lodQuality;
	extern int32_t g_viewNear;
	extern int32_t g_viewShiftX;
	extern int32_t g_viewShiftY;
	extern int32_t g_viewFar;
	extern int32_t g_viewLeft;
	extern int32_t g_viewCenterX;
	extern int32_t g_viewBottom;
	extern int32_t g_viewRight;
	extern int32_t g_viewTop;
	extern int32_t g_viewCenterY;
	extern int32_t g_viewFarPlane;
	extern int32_t g_viewProjX0;
	extern int32_t g_viewProjX1;
	extern int32_t g_viewProjX2;
	extern int32_t g_viewProjY0;
	extern int32_t g_viewProjY1;
	extern int32_t g_viewProjY2;
	extern int32_t g_viewProjZ0;
	extern int32_t g_viewProjZ1;
	extern int32_t g_viewProjZ2;
	extern int32_t g_viewRotX0;
	extern int32_t g_viewRotX1;
	extern int32_t g_viewRotX2;
	extern int32_t g_viewRotY0;
	extern int32_t g_viewRotY1;
	extern int32_t g_viewRotY2;
	extern int32_t g_viewRotZ0;
	extern int32_t g_viewRotZ1;
	extern int32_t g_viewRotZ2;
	extern int32_t g_viewEyeY;
	extern int32_t g_viewEyeX;
	extern int32_t g_viewEyeZ;
	extern int32_t g_viewLightZ;
	extern int32_t g_viewLightX;
	extern int32_t g_viewLightY;
	extern int32_t g_viewNearPlane;
	extern int32_t g_viewHalfHeight;
	extern int32_t g_viewHalfWidth;
	extern int32_t g_viewProjectScaleX;
	extern int32_t g_viewProjectScaleY;
	extern int32_t g_viewLeftScaled;
	extern int32_t g_viewBottomScaled;
	extern int32_t g_viewProjectScaleX16;
	extern int32_t g_viewProjectScaleY16;
	extern int32_t g_viewTopScaled;
	extern int32_t g_viewRightScaled;

	void SelectEyepoint(Eyepoint* p_eyepoint);
	void UpdateProjection(Eyepoint* p_eyepoint);
	void SetNearPlane(Eyepoint* p_eyepoint, int32_t p_value);
	void SetFarPlane(Eyepoint* p_eyepoint, int32_t p_value);
	void UpdateViewMatrix(Eyepoint* p_eyepoint);
	void ResetEyepointView(Eyepoint* p_eyepoint);
	void SetEyepointTransform(Eyepoint* p_eyepoint, Matrix* p_matrix);
	void GetEyepointTransform(Eyepoint* p_eyepoint, Matrix* p_matrix);
	int32_t ProjectWorldPoint(int32_t* p_x, int32_t* p_y, int32_t* p_z);
	int32_t CullSceneShape(struct Shape* p_shape);
	int32_t CullShapeToFrustum(struct Shape* p_shape);
	int32_t CullHiddenShape(struct Shape* p_shape);
	int32_t IsLodQualityHigh(void);
	void SetLodQualityHigh(int32_t p_enable);

#ifdef __cplusplus
}
#endif

#endif // VIEW_H
