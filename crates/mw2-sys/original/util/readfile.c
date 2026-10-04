#include "readfile.h"

#include <stdio.h>

void* MechReadFile(MechHeap* p_heap, const char* p_path)
{
	FILE* file;
	long size;
	void* data;

	file = fopen(p_path, "rb");
	if (file == NULL) {
		return NULL;
	}

	data = NULL;
	if (fseek(file, 0, SEEK_END) == 0 && (size = ftell(file)) >= 0 && fseek(file, 0, SEEK_SET) == 0) {
		data = MechHeapAlloc(p_heap, (size_t) size);
		if (data != NULL && fread(data, 1, (size_t) size, file) != (size_t) size) {
			MechHeapFree(p_heap, data);
			data = NULL;
		}
	}

	fclose(file);
	return data;
}
