#pragma once 

// Mac OS X 64bit or Ubuntu 64bit
#if (defined(__APPLE__) && defined(__MACH__) && defined(__x86_64__)) || (defined(__linux__) && defined(__x86_64__))
    #ifndef _INT8_T
    #define _INT8_T
        typedef signed char int8_t;
    #endif /* _INT8_T */

    #ifndef _UINT8_T
    #define _UINT8_T
        typedef unsigned char uint8_t;
    #endif /* _UINT8_T */

    #ifndef _INT16_T
    #define _INT16_T
        typedef short int16_t;
    #endif /* _INT16_T */

    #ifndef _UINT16_T
    #define _UINT16_T
        typedef unsigned short uint16_t;
    #endif /* _UINT16_T */

    #ifndef _INT32_T
    #define _INT32_T
        typedef int int32_t;
    #endif /* _INT32_T */

    #ifndef _UINT32_T
    #define _UINT32_T
        typedef unsigned int uint32_t;  
    #endif /* _UINT32_T */

    #ifndef _INT64_T
    #define _INT64_T
        typedef long int64_t;
    #endif /* _INT64_T */

    #ifndef _UINT64_T
    #define _UINT64_T
        typedef unsigned long uint64_t;
    #endif /* _UINT64_T */

    #ifndef _UINTPTR_T
    #define _UINTPTR_T
        typedef unsigned long uintptr_t;
    #endif /* _UINTPTR_T */

    #ifndef _INTPTR_T
    #define _INTPTR_T
        typedef long intptr_t;
    #endif /* _INTPTR_T */
#else
    #error "Unsupported platform"
#endif
