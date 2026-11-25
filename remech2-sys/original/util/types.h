#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h> // IWYU pragma: export
#include <stddef.h>  // IWYU pragma: export
#include <stdint.h>  // IWYU pragma: export

#define MECH_PTR_TO_S32(p_pointer) ((int32_t) (intptr_t) (p_pointer))
#define MECH_S32_TO_PTR(p_value) ((void*) (intptr_t) (p_value))

/* windows.h's max */
#define MECH_MAX(p_a, p_b) ((p_a) > (p_b) ? (p_a) : (p_b))

/* A position in pixels, laid out as Windows' POINT */
typedef struct MechPoint {
	int32_t x;
	int32_t y;
} MechPoint;

#endif /* TYPES_H */
