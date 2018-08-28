#pragma once

#include "templates.h"

namespace primitives
{

bool isDebuggerAttached();

}

#define PRIMITIVES_DO_WHILE(x) \
    do                         \
    {                          \
        x;                     \
    } while (0)

#if defined(_MSC_VER)

#define DEBUG_BREAK_MSVC()                                                \
    static auto ANONYMOUS_VARIABLE_LINE(primitives_debug) = 1;            \
    if (ANONYMOUS_VARIABLE_LINE(primitives_debug) && IsDebuggerPresent()) \
    DebugBreak()

#define DEBUG_BREAK PRIMITIVES_DO_WHILE(DEBUG_BREAK_MSVC())

#elif !defined(NDEBUG)

#define DEBUG_BREAK_INT3()                                                               \
    static auto ANONYMOUS_VARIABLE_LINE(primitives_debug) = 1;                           \
    if (ANONYMOUS_VARIABLE_LINE(primitives_debug) && ::primitives::isDebuggerAttached()) \
    __asm__("int3")

#define DEBUG_BREAK PRIMITIVES_DO_WHILE(DEBUG_BREAK_INT3())

#else

#define DEBUG_BREAK PRIMITIVES_DO_WHILE(;)

#endif

#define DEBUG_BREAK_IF(x) \
    if (x)                \
    DEBUG_BREAK

#define DEBUG_BREAK_IF_STRING_HAS(s, substr) \
    if (s.find(substr) != s.npos)            \
    DEBUG_BREAK

#define DEBUG_BREAK_IF_STRING_STARTS_WITH(s, substr) \
    if (s.find(substr) == 0)                         \
    DEBUG_BREAK

#define DEBUG_BREAK_IF_PATH_HAS(s, substr) \
    DEBUG_BREAK_IF_STRING_HAS(s.u8string(), substr)