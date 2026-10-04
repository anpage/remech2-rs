#ifndef TYPES_H
#define TYPES_H

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

/* Project type vocabulary. Typedefs are codegen-neutral; the underlying type
   (signed/unsigned, char/int, float/double) is what affects /Od codegen, so the
   width and signedness are explicit here. */

typedef signed char MechS8;
typedef unsigned char MechU8;
typedef short MechS16;
typedef unsigned short MechU16;
typedef int MechS32;
typedef unsigned int MechU32;
typedef float MechFloat;
typedef double MechDouble;
typedef char MechChar;

/* The integers the original keeps in pointers (a screen field's item index, a file offset) and
   the pointers it keeps in 32-bit integers (a Miles sample's user data) go through a
   pointer-sized integer, so that the conversions are defined where pointers are wider than 32
   bits. Macros, not typedefs: a typedef here is a symbol in every unit, and 4.1's operand order
   and stack slots react to the symbol count. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define MECH_INTPTR int
#else
#include <stdint.h>
#define MECH_INTPTR intptr_t
#endif

#define MECH_PTR_TO_S32(p_pointer) ((MechS32) (MECH_INTPTR) (p_pointer))
#define MECH_S32_TO_PTR(p_value) ((void*) (MECH_INTPTR) (p_value))

/* Boolean typedefs (MechBool / MechBool8 ...) are added when a match proves
   the width and signedness. */

#endif /* TYPES_H */
