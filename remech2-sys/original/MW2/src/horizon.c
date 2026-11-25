/* Conversions between the screen and the eyepoint's view, with the view matrix's second column. In
   the original, each function's products and quotients are an __asm block (imul/idiv on 64-bit
   intermediates). The portable C here replaces each whole function, and wraps where standard C
   overflows. */
#include "horizon.h"

#include "eyepoint.h"
#include "portable.h"
#include "types.h"

// Returns whether the screen point (p_x, p_y) lies above the horizon of p_eyepoint's view.
int32_t IsAboveHorizon(int32_t p_x, int32_t p_y, Eyepoint* p_eyepoint)
{
	int32_t offsetX = PortableS32((uint32_t) p_x - (uint32_t) p_eyepoint->m_centerX);
	int32_t offsetY = PortableS32((uint32_t) p_eyepoint->m_centerY - (uint32_t) p_y);
	uint32_t sum =
		(uint32_t)
			PortableIdiv((MechS64) offsetX * p_eyepoint->m_viewMatrix.m_rows[0][1], p_eyepoint->m_projectScaleX) +
		(uint32_t) PortableIdiv((MechS64) offsetY * p_eyepoint->m_viewMatrix.m_rows[1][1], p_eyepoint->m_projectScaleY);
	int32_t horizon = PortableSar32(PortableS32(0 - (uint32_t) p_eyepoint->m_viewMatrix.m_rows[2][1]), 16);

	return PortableS32(sum) >= horizon;
}

// Returns the screen y of the horizon at screen x p_x in p_eyepoint's view.
int32_t HorizonYAtX(int32_t p_x, Eyepoint* p_eyepoint)
{
	int32_t x = PortableS32((uint32_t) p_x - (uint32_t) p_eyepoint->m_centerX);
	uint32_t height =
		(uint32_t) PortableIdiv((MechS64) x * p_eyepoint->m_viewMatrix.m_rows[0][1], p_eyepoint->m_projectScaleX) +
		(uint32_t) PortableSar32(p_eyepoint->m_viewMatrix.m_rows[2][1], 16);
	uint32_t y = (uint32_t) PortableIdiv(
		(MechS64) PortableS32(0 - height) * p_eyepoint->m_projectScaleY,
		p_eyepoint->m_viewMatrix.m_rows[1][1]
	);

	return PortableS32((uint32_t) p_eyepoint->m_centerY - y);
}

// Returns the screen x of the horizon at screen y p_y in p_eyepoint's view.
int32_t HorizonXAtY(int32_t p_y, Eyepoint* p_eyepoint)
{
	int32_t y = PortableS32((uint32_t) p_eyepoint->m_centerY - (uint32_t) p_y);
	uint32_t width =
		(uint32_t) PortableIdiv((MechS64) y * p_eyepoint->m_viewMatrix.m_rows[1][1], p_eyepoint->m_projectScaleY) +
		(uint32_t) PortableSar32(p_eyepoint->m_viewMatrix.m_rows[2][1], 16);
	uint32_t x = (uint32_t) PortableIdiv(
		(MechS64) PortableS32(0 - width) * p_eyepoint->m_projectScaleX,
		p_eyepoint->m_viewMatrix.m_rows[0][1]
	);

	return PortableS32((uint32_t) p_eyepoint->m_centerX + x);
}
