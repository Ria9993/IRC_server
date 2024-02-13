#pragma once

#include <cassert>
#include <iostream>

#if !defined(__GNUC__) && !defined(_MSC_VER)
	#error "This compiler is not supported"
#endif

/** 
 *	Implemented as direct interrupt to avoid dirtying the call stack with assert function when debugging.
 *	And for optimization, change to Assume() in release builds.
 */
#if defined(__GNUC__)
	#if defined(NDEBUG)
		#define Assert(exp, msg) do { if (!(exp)) __builtin_unreachable(); } while (0)
		#define Assert(exp) do { if (!(exp)) __builtin_unreachable(); } while (0)
	#else
		#define Assert(exp, msg) do { if (!(exp)) { std::cerr << "Assertion failed: " << #exp << " " << msg << " at " << __FILE__ << ":" << __LINE__ << std::endl; __builtin_trap(); } } while (0)
		#define Assert(exp) do { if (!(exp)) { std::cerr << "Assertion failed: " << #exp << " at " << __FILE__ << ":" << __LINE__ << std::endl; __builtin_trap(); } } while (0)
	#endif
#elif defined(_MSC_VER)
	#if defined(NDEBUG)
		#define Assert(exp, msg) do { if (!(exp)) __assume(0); } while (0)
		#define Assert(exp) do { if (!(exp)) __assume(0); } while (0)
	#else
		#define Assert(exp, msg) do { if (!(exp)) { std::cerr << "Assertion failed: " << #exp << " " << msg << " at " << __FILE__ << ":" << __LINE__ << std::endl; __debugbreak(); } } while (0)
		#define Assert(exp) do { if (!(exp)) { std::cerr << "Assertion failed: " << #exp << " at " << __FILE__ << ":" << __LINE__ << std::endl; __debugbreak(); } } while (0)
	#endif
#else
	#if defined(NDEBUG)
		#define Assert(exp, msg) assert(exp)
		#define Assert(exp) assert(exp)
	#else
		#define Assert(exp, msg) do { if (!(exp)) { std::cerr << "Assertion failed: " << #exp << " " << msg << " at " << __FILE__ << ":" << __LINE__ << std::endl; assert(exp); } } while (0)
		#define Assert(exp) do { if (!(exp)) { std::cerr << "Assertion failed: " << #exp << " at " << __FILE__ << ":" << __LINE__ << std::endl; assert(exp); } } while (0)
	#endif
#endif

#if defined(__GNUC__)
	#define Assume(x) do { if (!(x)) __builtin_unreachable(); } while (0)
#elif defined(_MSC_VER)
	#define Assume(x) __assume(x)
#else
	#define Assume(x)
#endif
