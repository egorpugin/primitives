#pragma once

#include <string>

namespace primitives
{

PRIMITIVES_DEBUG_API
bool isDebuggerAttached();

PRIMITIVES_DEBUG_API
void debugBreak();

PRIMITIVES_DEBUG_API
std::string getThreadName();

PRIMITIVES_DEBUG_API
std::string setThreadName(const std::string &name);

// returns old name
inline std::string appendThreadName(const std::string &name)
{
    return setThreadName(getThreadName() + name);
}

struct ScopedThreadName
{
    ScopedThreadName(const std::string &name, bool append = false)
        : dbg(true /*primitives::isDebuggerAttached()*/)
    {
        if (dbg)
            old_thread_name = append ? appendThreadName(name) : setThreadName(name);
    }
    ~ScopedThreadName()
    {
        if (dbg)
            setThreadName(old_thread_name);
    }

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

#if !defined(NDEBUG) || _MSC_VER

#define DEBUG_BREAK_INTERNAL()                                                           \
    static auto ANONYMOUS_VARIABLE_LINE(primitives_debug) = 1;                           \
    if (::primitives::isDebuggerAttached() && ANONYMOUS_VARIABLE_LINE(primitives_debug)) \
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
