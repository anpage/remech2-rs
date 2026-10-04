#ifndef DECOMP_H
#define DECOMP_H

#ifndef NDEBUG
// Disable size assertions for debug builds because the sizes differ between debug and release
// builds. The original MW2.DLL and MW2SHELL.DLL are what we ultimately want to match, so this is
// what we assert against.
#undef ENABLE_DECOMP_ASSERTS
#endif

// The sizes are the originals', where pointers are 32-bit: elsewhere a struct with pointers grows.
// Decided by the preprocessor: a sizeof(void*) in every assertion changes 4.1's C++ codegen.
#if defined(_WIN64) || (defined(__SIZEOF_POINTER__) && __SIZEOF_POINTER__ != 4)
#undef ENABLE_DECOMP_ASSERTS
#endif

#if defined(ENABLE_DECOMP_ASSERTS)
// C-compatible static assert: the typedef emits no code in both C and C++.
// The double expansion makes __LINE__ resolve before the tokens are pasted,
// so each assertion gets a unique typedef name.
#define DECOMP_STATIC_ASSERT_CONCAT_IMPL(a, b) a##b
#define DECOMP_STATIC_ASSERT_CONCAT(a, b) DECOMP_STATIC_ASSERT_CONCAT_IMPL(a, b)
#define DECOMP_STATIC_ASSERT(V) typedef char DECOMP_STATIC_ASSERT_CONCAT(decomp_static_assert_, __LINE__)[(V) ? 1 : -1];
#define DECOMP_SIZE_ASSERT(T, S) DECOMP_STATIC_ASSERT(sizeof(T) == S)
#else
#define DECOMP_STATIC_ASSERT(V)
#define DECOMP_SIZE_ASSERT(T, S)
#endif

#ifndef sizeOfArray
#define sizeOfArray(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

typedef unsigned char undefined;
typedef unsigned short undefined2;
typedef unsigned int undefined4;

// C linkage so that STUB()/MATCHING() in .c files link against the definition
// in decomp.cpp (a C++ translation unit would otherwise decorate the name).
#ifdef __cplusplus
extern "C"
{
#endif

	extern undefined4 g_foldingDummyVariable;

#ifdef __cplusplus
}
#endif

// Use STUB when a function has not yet been matched and/or is WIP.
#define STUB(val)                                                                                                      \
	do {                                                                                                               \
		g_foldingDummyVariable = val;                                                                                  \
	} while (0)

// Use MATCHING when a function is matched, but function folding inhibits success.
#define MATCHING(val)                                                                                                  \
	do {                                                                                                               \
		g_foldingDummyVariable = val;                                                                                  \
	} while (0)

#endif // DECOMP_H
