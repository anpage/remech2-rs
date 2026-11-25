/* In the original, IntegrateMidpoint is a C function whose body is an __asm block. This is portable
   C in its place. */
#include "integrate.h"

#include "portable.h"
#include "types.h"

// Advances p_velocity by p_acceleration * p_time and p_position by the midpoint velocity
// times p_time (16.16 fixed point, with a 64-bit product).
void IntegrateMidpoint(int32_t* p_position, int32_t* p_velocity, int32_t p_acceleration, int32_t p_time)
{
	/* Only the product's low dword reaches the velocity and the midpoint; the sums wrap. */
	uint32_t delta = (uint32_t) ((MechS64) p_acceleration * p_time);
	uint32_t velocity = (uint32_t) *p_velocity;
	int32_t midpoint = PortableS32((uint32_t) PortableSar32(PortableS32(delta), 1) + velocity);

	*p_velocity = PortableS32(velocity + delta);
	*p_position = PortableS32((uint32_t) *p_position + (uint32_t) ((MechU64) ((MechS64) midpoint * p_time) >> 16));
}
