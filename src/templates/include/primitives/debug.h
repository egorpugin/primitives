#pragma once

#include "templates.h"

#include <primitives/string.h>

#ifndef _WIN32
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#else
#include <winsock2.h>
//#include <Windows.h>
#endif

/*#ifdef _WIN32
// APIs was brought in version 1607 of Windows 10.
typedef HRESULT(WINAPI* GetThreadDescription)(HANDLE hThread,
    PWSTR *ppszThreadDescription);
typedef HRESULT(WINAPI* SetThreadDescription)(HANDLE hThread,
    PCWSTR lpThreadDescription);
#endif*/

namespace primitives
{

inline bool isDebuggerAttached()
{
#ifdef _WIN32
    return IsDebuggerPresent();
#else
    char buf[4096];

    const int status_fd = ::open("/proc/self/status", O_RDONLY);
    if (status_fd == -1)
        return false;

    const ssize_t num_read = ::read(status_fd, buf, sizeof(buf) - 1);
    if (num_read <= 0)
        return false;

    buf[num_read] = '\0';
    constexpr char tracerPidString[] = "TracerPid:";
    const auto tracer_pid_ptr = ::strstr(buf, tracerPidString);
    if (!tracer_pid_ptr)
        return false;

    for (const char* characterPtr = tracer_pid_ptr + sizeof(tracerPidString) - 1; characterPtr <= buf + num_read; ++characterPtr)
    {
        if (::isspace(*characterPtr))
            continue;
        else
            return ::isdigit(*characterPtr) != 0 && *characterPtr != '0';
    }
    return false;
#endif
}

inline void debugBreak()
{
#ifdef _WIN32
    DebugBreak();
#elif defined(__aarch64__)
    __asm__("brk #0x1"); // "trap" does not work for gcc
#else
    __asm__("int3");
#endif
}

inline std::string getThreadName()
{
    // disable for now, very very very slow
    return {};

    /*std::string name;
#ifdef _WIN32
    auto get_thread_description_func =
        reinterpret_cast<GetThreadDescription>(::GetProcAddress(
            ::GetModuleHandleW(L"Kernel32.dll"), "GetThreadDescription"));
    if (get_thread_description_func)
    {
        PWSTR data;
        auto hr = get_thread_description_func(::GetCurrentThread(), &data);
        if (SUCCEEDED(hr))
        {
            name = to_string(std::wstring(data));
            LocalFree(data);
        }
    }
#endif
    return name;*/
}

inline std::string setThreadName(const std::string &name)
{
    // disable for now, very very very slow
    return {};

    /*auto old = getThreadName();

#ifdef _WIN32
    // The SetThreadDescription API works even if no debugger is attached.
    auto set_thread_description_func =
        reinterpret_cast<SetThreadDescription>(::GetProcAddress(
            ::GetModuleHandleW(L"Kernel32.dll"), "SetThreadDescription"));
    if (set_thread_description_func) {
        set_thread_description_func(::GetCurrentThread(),
            to_wstring(name).c_str());
    }
#else
    //pthread_setname_np(thread_pool[i].t.native_handle(), name.c_str());
#endif

    return old;*/
}


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
