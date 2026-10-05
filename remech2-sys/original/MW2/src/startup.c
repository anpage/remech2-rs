#include "startup.h"

#include "decomp.h"
#include "types.h"

#include <stdio.h>

// Prints the revision strings of the simulator and of its project file.
// FUNCTION: MW2 0x10071790
void PrintVersion(void)
{
	printf("\n%s\n", "MW2 FM $Name: BETA_PATCH $            $Revision: 1.61 $ $Date: 1995/06/25 15:47:38 $");
	printf(
		"%s\n",
		"Project File $Name: BETA_PATCH $                        $Revision: 1.22 $ $Date: 1995/05/16 21:41:39 $"
	);
}

// Always returns 0. SimMain raises Error(0x51) if it doesn't, so it's a stubbed-out startup check.
// FUNCTION: MW2 0x100717bf
MechS32 StartupCheckStub(void)
{
	return 0;
}
