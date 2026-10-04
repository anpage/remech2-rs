#ifndef COMPAT_H
#define COMPAT_H

// Various macros to enable compiling with other/newer compilers.

// Visual C++ 4.1 (the original toolchain) -> _MSC_VER 1010
#define MSVC410_VERSION 1010

#if defined(__MINGW32__) || defined(__clang__) || defined(__GNUC__) || (defined(_MSC_VER) && _MSC_VER > MSVC410_VERSION)
#define COMPAT_MODE
#endif

// Hand-written assembly inside compiled units has a portable C replacement (util/portable.h,
// tested against the assembly by tests/asmequiv), which every COMPAT_MODE build compiles. A
// VC++ 4.1 build selects it with /DPORTABLE_C (the tests' 4.1 candidate), and an x86 MSVC
// build keeps the assembly with /DREFERENCE_ASM (their modern reference).
#if defined(COMPAT_MODE) && !defined(REFERENCE_ASM) && !defined(PORTABLE_C)
#define PORTABLE_C
#endif

#if defined(_MSC_VER)
// Disable "identifier was truncated to '255' characters" warning.
// Impossible to avoid this if using STL map or set.
// This removes most (but not all) occurrences of the warning.
#pragma warning(disable : 4786)
#endif

// We use `override` so newer compilers can tell us our vtables are valid,
// however this keyword was added in C++11, so we define it as empty for
// compatibility with older compilers. C has no such keyword, so the shims
// are only defined for C++.
#if defined(__cplusplus) && __cplusplus < 201103L
#define override
#define static_assert(expr, msg)
#elif defined(__cplusplus)
#define override override
#endif

#if !defined(_MSC_VER)
#define __try if (1)
#define __finally if (1)
#undef AbnormalTermination
#define AbnormalTermination() 0
#endif

#endif // COMPAT_H
