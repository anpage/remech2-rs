#ifndef COMPAT_H
#define COMPAT_H

// Macros for building the decompiled code with gcc and clang

// Structured exception handling, which only MSVC has
#define __try if (1)
#define __finally if (1)
#undef AbnormalTermination
#define AbnormalTermination() 0

#endif // COMPAT_H
