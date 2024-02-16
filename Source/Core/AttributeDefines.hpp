#pragma once

#if !defined(__GNUC__) && !defined(_MSC_VER)
    #error "This compiler is not supported"
#endif

#if defined(__GNUC__)
    #define NODISCARD __attribute__((warn_unused_result))
#elif defined(_MSC_VER)
// [[nodiscard]] Visual Studio 2017 version 15.3 and later: (Available with /std:c++17 and later.)
    #if _MSC_VER >= 1911
        #define NODISCARD [[nodiscard]]
    #else
        #define NODISCARD
    #endif
#else
    #define NODISCARD
#endif

#if defined(__GNUC__)
    #define UNUSED __attribute__((unused))
#elif defined(_MSC_VER)
// [[[maybe_unused]] enable Visual Studio 2017 version 15.3 and later: (Available with /std:c++17 and later.) 
    #if _MSC_VER >= 1911
        #define UNUSED [[maybe_unused]]
    #else
        #define UNUSED
    #endif
#else
    #define UNUSED
#endif

#if defined(__GNUC__)
    #define LIKELY(x) __builtin_expect(!!(x), 1)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
#elif defined(_MSC_VER)
// [[likely]] enable Visual Studio 2019 version 16.6 and later: (Available with /std:c++20 and later.)
    #if _MSC_VER >= 1926
        #define LIKELY(x) [[likely]] (x)
        #define UNLIKELY(x) [[unlikely]] (x)
    #else
        #define LIKELY(x) (x)
        #define UNLIKELY(x) (x)
    #endif
#else
    #define LIKELY(x) (x)
    #define UNLIKELY(x) (x)
#endif

#if defined(__GNUC__)
    #define FORCEINLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
    #define FORCEINLINE __forceinline
#else
    #define FORCEINLINE inline
#endif

#if defined(__GNUC__)
    #define NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
    #define NOINLINE __declspec(noinline)
#else
    #define NOINLINE
#endif

#if defined(__GNUC__)
    #define ALIGNAS(x) __attribute__((aligned(x)))
#elif defined(_MSC_VER)
    #define ALIGNAS(x) __declspec(align(x))
#else
    #error "This compiler is not supported"
#endif

#if defined(__GNUC__)
    #define NORETURN __attribute__((noreturn))
#elif defined(_MSC_VER)
    #define NORETURN __declspec(noreturn)
#else
    #define NORETURN
#endif
