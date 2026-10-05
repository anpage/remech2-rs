/* Conversions between the screen and the eyepoint's view, with the view matrix's second column. In
   the original, each function's products and quotients are an __asm block (imul/idiv on 64-bit
   intermediates). The portable C here replaces each whole function, and wraps where standard C
   overflows. */
#include "horizon.h"

#include "eyepoint.h"
#include "portable.h"
#include "types.h"

// Returns whether the screen point (p_x, p_y) lies above the horizon of p_eyepoint's view.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x10071930
MechS32 IsAboveHorizon(MechS32 p_x, MechS32 p_y, Eyepoint* p_eyepoint)
{
	MechS32 offsetX = PortableS32((MechU32) p_x - (MechU32) p_eyepoint->m_centerX);
	MechS32 offsetY = PortableS32((MechU32) p_eyepoint->m_centerY - (MechU32) p_y);
	MechU32 sum =
		(MechU32) PortableIdiv((MechS64) offsetX * p_eyepoint->m_viewMatrix.m_rows[0][1], p_eyepoint->m_projectScaleX) +
		(MechU32) PortableIdiv((MechS64) offsetY * p_eyepoint->m_viewMatrix.m_rows[1][1], p_eyepoint->m_projectScaleY);
	MechS32 horizon = PortableSar32(PortableS32(0 - (MechU32) p_eyepoint->m_viewMatrix.m_rows[2][1]), 16);

	return PortableS32(sum) >= horizon;
}

// Returns the screen y of the horizon at screen x p_x in p_eyepoint's view.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x100719ca
MechS32 HorizonYAtX(MechS32 p_x, Eyepoint* p_eyepoint)
{
	MechS32 x = PortableS32((MechU32) p_x - (MechU32) p_eyepoint->m_centerX);
	MechU32 height =
		(MechU32) PortableIdiv((MechS64) x * p_eyepoint->m_viewMatrix.m_rows[0][1], p_eyepoint->m_projectScaleX) +
		(MechU32) PortableSar32(p_eyepoint->m_viewMatrix.m_rows[2][1], 16);
	MechU32 y = (MechU32) PortableIdiv(
		(MechS64) PortableS32(0 - height) * p_eyepoint->m_projectScaleY,
		p_eyepoint->m_viewMatrix.m_rows[1][1]
	);

	return PortableS32((MechU32) p_eyepoint->m_centerY - y);
}

// Returns the screen x of the horizon at screen y p_y in p_eyepoint's view.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x10071a4c
MechS32 HorizonXAtY(MechS32 p_y, Eyepoint* p_eyepoint)
{
	MechS32 y = PortableS32((MechU32) p_eyepoint->m_centerY - (MechU32) p_y);
	MechU32 width =
		(MechU32) PortableIdiv((MechS64) y * p_eyepoint->m_viewMatrix.m_rows[1][1], p_eyepoint->m_projectScaleY) +
		(MechU32) PortableSar32(p_eyepoint->m_viewMatrix.m_rows[2][1], 16);
	MechU32 x = (MechU32) PortableIdiv(
		(MechS64) PortableS32(0 - width) * p_eyepoint->m_projectScaleX,
		p_eyepoint->m_viewMatrix.m_rows[0][1]
	);

	return PortableS32((MechU32) p_eyepoint->m_centerX + x);
}
