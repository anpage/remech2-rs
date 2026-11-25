#include "palidentity.h"

#include "menu.h"
#include "types.h"

// Fills g_textColors with the identity mapping of the 256 palette indices.
void ResetTextColors(void)
{
	int32_t i;

	for (i = 0; i < 0x100; i++) {
		g_textColors[i] = i;
	}
}
