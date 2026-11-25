#include "stringutil.h"

#include "types.h"
#include "windowstate.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Returns a copy of the string on the primary heap, or NULL for an empty string.
char* AllocateString(char* p_string)
{
	char* copy;

	if (strlen(p_string) == 0) {
		return NULL;
	}

	copy = (char*) MechHeapAlloc(g_primaryHeap, strlen(p_string) + 1);
	if (copy == NULL) {
		fprintf(stderr, "Out of memory in allocate string\n");
		fflush(stderr);
		exit(1);
	}

	strcpy(copy, p_string);
	return copy;
}

// Uppercases a string in place and returns the same pointer.
char* UppercaseString(char* p_string)
{
	uint32_t i;

	for (i = 0; i < strlen(p_string); i++) {
		p_string[i] = toupper(p_string[i]);
	}

	return p_string;
}
