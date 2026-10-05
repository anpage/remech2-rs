/* In the original, IntegrateMidpoint is a C function whose body is an __asm block. This is portable
   C in its place. */
#include "integrate.h"

#include "portable.h"
#include "types.h"

// Advances p_velocity by p_acceleration * p_time and p_position by the midpoint velocity
// times p_time (16.16 fixed point, with a 64-bit product).
// FUNCTION: MW2 0x10004e90
void IntegrateMidpoint(MechS32* p_position, MechS32* p_velocity, MechS32 p_acceleration, MechS32 p_time)
{
	/* Only the product's low dword reaches the velocity and the midpoint; the sums wrap. */
	MechU32 delta = (MechU32) ((MechS64) p_acceleration * p_time);
	MechU32 velocity = (MechU32) *p_velocity;
	MechS32 midpoint = PortableS32((MechU32) PortableSar32(PortableS32(delta), 1) + velocity);

	*p_velocity = PortableS32(velocity + delta);
	*p_position = PortableS32((MechU32) *p_position + (MechU32) ((MechU64) ((MechS64) midpoint * p_time) >> 16));
}
