#include "debugbreak.h"

/* Hand-written assembly: DebugBreakpoint's body is an inline int 3 (its own object). */

// FUNCTION: MW2 0x1000a9b0
void DebugBreakpoint(void){
#if defined(_MSC_VER) && defined(_M_IX86)
	__asm int 3
#endif
}
