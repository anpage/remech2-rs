/* Conversions between the screen and the eyepoint's view, with the view matrix's second column.
   Hand-written assembly: each function's products and quotients are an __asm block (imul/idiv on
   64-bit intermediates). Their portable C (PORTABLE_C) is tested against the assembly by
   tests/asmequiv: it replaces each whole function, whose C wraps where standard C overflows. */
#include "horizon.h"

#include "compat.h"
#include "eyepoint.h"
#include "portable.h"
#include "types.h"

/* IsAboveHorizon's __asm block jumps to a C label, which newer compilers reject: their reference
   build (REFERENCE_ASM) compiles the portable C too. */
#pragma warning(disable : 4102) /* a label only the __asm block jumps to */

// Returns whether the screen point (p_x, p_y) lies above the horizon of p_eyepoint's view.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x10071930
MechS32 IsAboveHorizon(MechS32 p_x, MechS32 p_y, Eyepoint* p_eyepoint)
{
#if defined(PORTABLE_C) || !defined(_MSC_VER) || _MSC_VER >= 1100
	MechS32 offsetX = PortableS32((MechU32) p_x - (MechU32) p_eyepoint->m_centerX);
	MechS32 offsetY = PortableS32((MechU32) p_eyepoint->m_centerY - (MechU32) p_y);
	MechU32 sum =
		(MechU32) PortableIdiv((MechS64) offsetX * p_eyepoint->m_viewMatrix.m_rows[0][1], p_eyepoint->m_projectScaleX) +
		(MechU32) PortableIdiv((MechS64) offsetY * p_eyepoint->m_viewMatrix.m_rows[1][1], p_eyepoint->m_projectScaleY);
	MechS32 horizon = PortableSar32(PortableS32(0 - (MechU32) p_eyepoint->m_viewMatrix.m_rows[2][1]), 16);

	return PortableS32(sum) >= horizon;
#else
	MechS32 offsetX;
	MechS32 offsetY;
	MechS32 b;
	MechS32 d;
	MechS32 a;
	MechS32 c;
	MechS32 e;

	offsetX = p_x - p_eyepoint->m_centerX;
	offsetY = -p_y + p_eyepoint->m_centerY;
	b = p_eyepoint->m_projectScaleX;
	d = p_eyepoint->m_projectScaleY;
	a = p_eyepoint->m_viewMatrix.m_rows[0][1];
	c = p_eyepoint->m_viewMatrix.m_rows[1][1];
	e = p_eyepoint->m_viewMatrix.m_rows[2][1];
	__asm {
		mov eax, offsetX
		imul a
		idiv b
		mov ecx, eax
		mov eax, offsetY
		imul c
		idiv d
		add ecx, eax
		mov eax, e
		neg eax
		sar eax, 16
		cmp ecx, eax
		jge below
	}
	return FALSE;

below:
	return TRUE;
#endif
}

// Returns the screen y of the horizon at screen x p_x in p_eyepoint's view.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x100719ca
MechS32 HorizonYAtX(MechS32 p_x, Eyepoint* p_eyepoint)
{
#ifdef PORTABLE_C
	MechS32 x = PortableS32((MechU32) p_x - (MechU32) p_eyepoint->m_centerX);
	MechU32 height =
		(MechU32) PortableIdiv((MechS64) x * p_eyepoint->m_viewMatrix.m_rows[0][1], p_eyepoint->m_projectScaleX) +
		(MechU32) PortableSar32(p_eyepoint->m_viewMatrix.m_rows[2][1], 16);
	MechU32 y = (MechU32) PortableIdiv(
		(MechS64) PortableS32(0 - height) * p_eyepoint->m_projectScaleY,
		p_eyepoint->m_viewMatrix.m_rows[1][1]
	);

	return PortableS32((MechU32) p_eyepoint->m_centerY - y);
#else
	MechS32 y;
	MechS32 a;
	MechS32 b;
	MechS32 d;
	MechS32 c;
	MechS32 e;

	b = p_eyepoint->m_projectScaleX;
	d = p_eyepoint->m_projectScaleY;
	a = p_eyepoint->m_viewMatrix.m_rows[0][1];
	c = p_eyepoint->m_viewMatrix.m_rows[1][1];
	e = p_eyepoint->m_viewMatrix.m_rows[2][1];
	p_x -= p_eyepoint->m_centerX;
	__asm {
		mov eax, p_x
		imul a
		idiv b
		mov edx, e
		sar edx, 16
		add eax, edx
		neg eax
		imul d
		idiv c
		mov y, eax
	}
	return -y + p_eyepoint->m_centerY;
#endif
}

// Returns the screen x of the horizon at screen y p_y in p_eyepoint's view.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x10071a4c
MechS32 HorizonXAtY(MechS32 p_y, Eyepoint* p_eyepoint)
{
#ifdef PORTABLE_C
	MechS32 y = PortableS32((MechU32) p_eyepoint->m_centerY - (MechU32) p_y);
	MechU32 width =
		(MechU32) PortableIdiv((MechS64) y * p_eyepoint->m_viewMatrix.m_rows[1][1], p_eyepoint->m_projectScaleY) +
		(MechU32) PortableSar32(p_eyepoint->m_viewMatrix.m_rows[2][1], 16);
	MechU32 x = (MechU32) PortableIdiv(
		(MechS64) PortableS32(0 - width) * p_eyepoint->m_projectScaleX,
		p_eyepoint->m_viewMatrix.m_rows[0][1]
	);

	return PortableS32((MechU32) p_eyepoint->m_centerX + x);
#else
	MechS32 x;
	MechS32 a;
	MechS32 b;
	MechS32 d;
	MechS32 c;
	MechS32 e;

	b = p_eyepoint->m_projectScaleX;
	d = p_eyepoint->m_projectScaleY;
	a = p_eyepoint->m_viewMatrix.m_rows[0][1];
	c = p_eyepoint->m_viewMatrix.m_rows[1][1];
	e = p_eyepoint->m_viewMatrix.m_rows[2][1];
	p_y = -p_y + p_eyepoint->m_centerY;
	__asm {
		mov eax, p_y
		imul c
		idiv d
		mov edx, e
		sar edx, 16
		add eax, edx
		neg eax
		imul b
		idiv a
		mov x, eax
	}
	return p_eyepoint->m_centerX + x;
#endif
}
