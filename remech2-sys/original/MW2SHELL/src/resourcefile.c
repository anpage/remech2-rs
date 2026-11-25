#include "resourcefile.h"

#include "files.h"
#include "log.h"
#include "types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Resource file helpers: loading a file whole, and prefixing names with the base directory.

// Optional base directory for resource names.
char g_resourceDir[0x100] = {0};

// Shared scratch buffer returned by the path helpers (overwritten on each call).
char g_resourcePath[0x50];

// Read a whole file into memory, allocated here unless p_preallocated; returns the open handle,
// or -1 on failure.
int32_t LoadFile(char* p_name, int32_t* p_size, void** p_data, int32_t p_preallocated)
{
	int32_t handle;

	handle = MechOpen(p_name, c_mechOpenRead);
	if (handle == -1) {
		MechLogErrorf("symlog.txt: Couldn't load ID=%s\n", p_name);
		return -1;
	}

	*p_size = MechFileLength(handle);
	if (!p_preallocated) {
		*p_data = malloc(*p_size);
	}
	if (*p_data == NULL) {
		MechClose(handle);
		return -1;
	}

	if (MechRead(handle, *p_data, *p_size) != *p_size) {
		if (!p_preallocated) {
			free(*p_data);
		}
		MechClose(handle);
		return -1;
	}

	return handle;
}

// Prefix a bare name with the base directory; leave paths containing a separator unchanged.
char* MakeResourcePath(char* p_name)
{
	int32_t i;

	for (i = 0; i < 0x50; i++) {
		g_resourcePath[i] = '\0';
	}

	if (g_resourceDir[0] != '\0' && strchr(p_name, '\\') == NULL && strchr(p_name, '/') == NULL) {
		snprintf(g_resourcePath, sizeof(g_resourcePath), "%s\\%s", g_resourceDir, p_name);
	}
	else {
		strcpy(g_resourcePath, p_name);
	}

	return g_resourcePath;
}

// Prefix a name with the base directory without checking for path separators.
char* MakeResourcePathUnchecked(char* p_name)
{
	int32_t i;

	for (i = 0; i < 0x50; i++) {
		g_resourcePath[i] = '\0';
	}

	if (g_resourceDir[0] != '\0') {
		snprintf(g_resourcePath, sizeof(g_resourcePath), "%s\\%s", g_resourceDir, p_name);
	}
	else {
		strcpy(g_resourcePath, p_name);
	}

	return g_resourcePath;
}
