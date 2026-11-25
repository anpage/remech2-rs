#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#define MECH_INTPTR intptr_t

#define MECH_PTR_TO_S32(p_pointer) ((int32_t) (MECH_INTPTR) (p_pointer))
#define MECH_S32_TO_PTR(p_value) ((void*) (MECH_INTPTR) (p_value))

/* windows.h's max */
#define MECH_MAX(p_a, p_b) ((p_a) > (p_b) ? (p_a) : (p_b))

/* A position in pixels, laid out as Windows' POINT */
typedef struct MechPoint {
	int32_t x;
	int32_t y;
} MechPoint;

#endif /* TYPES_H */
