#pragma once

#include "templates.h"

namespace primitives
{

bool isDebuggerAttached();
void debugBreak();

std::string getThreadName();

// returns old name
std::string appendThreadName(const std::string &name);
std::string setThreadName(const std::string &name);

struct ScopedThreadName
{
    ScopedThreadName(const std::string &name, bool append = false);
    ~ScopedThreadName();

private:
    bool dbg;
    std::string old_thread_name;
};

}

#define PRIMITIVES_DO_WHILE(x) \
    do                         \
    {                          \
        x;                     \
    } while (0)

#if !defined(NDEBUG)

#define DEBUG_BREAK_INTERNAL()                                                           \
    static auto ANONYMOUS_VARIABLE_LINE(primitives_debug) = 1;                           \
    if (ANONYMOUS_VARIABLE_LINE(primitives_debug) && ::primitives::isDebuggerAttached()) \
    ::primitives::debugBreak()

#define DEBUG_BREAK PRIMITIVES_DO_WHILE(DEBUG_BREAK_INTERNAL())

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
