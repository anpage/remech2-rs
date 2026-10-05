#ifndef READFILE_H
#define READFILE_H

#include "heap.h"

#ifdef __cplusplus
extern "C" {
#endif

void *MechReadFile(MechHeap *p_heap, const char *p_path);

#ifdef __cplusplus
}
#endif

#endif // READFILE_H
