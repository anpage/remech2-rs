/* ticks.asm's routines and data for builds with other compilers (COMPAT_MODE): portable C, tested
   against the assembly by tests/asmequiv. The VC++ 4.1 build assembles ticks.asm with MASM 6.11
   instead. */
#include "ticks.h"

#include "compat.h"
#include "portable.h"
#include "types.h"

#define TICKS_HANDLES 64
#define TICKS_FIRST 0x80 // a handle's (or AllocTicks's flags') bit for the first counter

MechU32 g_ticksPaused = 0;
MechS32 g_ticks1Bases[TICKS_HANDLES] = {0};
MechS32 g_ticks2Bases[TICKS_HANDLES] = {0};
MechS32 g_ticks1 = 0;
MechS32 g_ticks2 = 0;

// The start value of a handle: its slot in its counter's table. The handle's other bits index
// the table too; the game's handles have none.
static MechS32* TickBase(MechU32 p_handle)
{
	PORTABLE_ASSERT((p_handle & ~(MechU32) TICKS_FIRST) < TICKS_HANDLES);
	if (p_handle & TICKS_FIRST) {
		return &g_ticks1Bases[p_handle & ~(MechU32) TICKS_FIRST];
	}

	return &g_ticks2Bases[p_handle];
}

static MechS32 TickCounter(MechU32 p_handle)
{
	return p_handle & TICKS_FIRST ? g_ticks1 : g_ticks2;
}

void GameTickTimerCallback(void)
{
	if (!(g_ticksPaused & 0x200)) {
		g_ticks1 = PortableS32((MechU32) g_ticks1 + 1);
	}

	if (!(g_ticksPaused & 0x100)) {
		g_ticks2 = PortableS32((MechU32) g_ticks2 + 1);
	}
}

// A slot is free at 0, and -1 ends the search. The original searches past the table when it
// finds neither, which the game, with at most a few handles, never makes it do; here the search
// stops at the table's end, as at -1 (and asserts, in the tests and debug builds).
MechS16 AllocTicks(MechU32 p_flags)
{
	MechS32* bases = p_flags & TICKS_FIRST ? g_ticks1Bases : g_ticks2Bases;
	MechS32 ticks = p_flags & TICKS_FIRST ? g_ticks1 : g_ticks2;
	MechS32 slot;

	for (slot = 0; slot < TICKS_HANDLES && bases[slot] != 0; slot++) {
		if (bases[slot] == -1) {
			return -1;
		}
	}

	PORTABLE_ASSERT(slot < TICKS_HANDLES);
	if (slot == TICKS_HANDLES) {
		return -1;
	}

	/* 0 marks a free slot, so a counter at 0 starts the handle at 1 */
	bases[slot] = ticks ? ticks : 1;
	return (MechS16) (p_flags & TICKS_FIRST ? slot | TICKS_FIRST : slot);
}

MechS32 GetTicks(MechU32 p_handle)
{
	return PortableS32((MechU32) TickCounter(p_handle) - (MechU32) *TickBase(p_handle));
}

void ResetTicks(MechU32 p_handle)
{
	*TickBase(p_handle) = TickCounter(p_handle);
}

void SetTicks(MechU32 p_handle, MechS32 p_ticks)
{
	*TickBase(p_handle) = PortableS32((MechU32) TickCounter(p_handle) - (MechU32) p_ticks);
}

void FreeTicks(MechU32 p_handle)
{
	*TickBase(p_handle) = 0;
}

// Only the low words of the arguments count.
void PauseTimer(MechS32 p_flags, MechS32 p_paused)
{
	MechU32 bits = 0;

	if (p_flags & TICKS_FIRST) {
		bits |= 0x200;
	}

	if (p_flags & 0x100) {
		bits |= 0x100;
	}

	if ((MechU16) p_paused) {
		g_ticksPaused |= bits;
	}
	else {
		g_ticksPaused &= ~bits;
	}
}
