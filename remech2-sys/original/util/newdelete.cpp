#include <stdlib.h>

void *operator new(size_t p_size) { return malloc(p_size ? p_size : 1); }

void operator delete(void *p_ptr) noexcept { free(p_ptr); }

void operator delete(void *p_ptr, size_t) noexcept { free(p_ptr); }
