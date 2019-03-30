#include <primitives/debug.h>

#include <codecvt>
#include <locale>
#include <string>

#ifndef _WIN32
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#else
#include <windows.h>
#endif

static auto& get_string_converter()
{
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter;
}

static std::string to_string(const std::wstring &s)
{
    auto &converter = get_string_converter();
    return converter.to_bytes(s.c_str());
}

static std::wstring to_wstring(const std::string &s)
{
    auto &converter = get_string_converter();
    return converter.from_bytes(s.c_str());
}

namespace primitives
{

bool isDebuggerAttached()
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
#endif
    return false;
}

#ifdef _WIN32

// APIs was brought in version 1607 of Windows 10.
typedef HRESULT(WINAPI* GetThreadDescription)(HANDLE hThread,
    PWSTR *ppszThreadDescription);
typedef HRESULT(WINAPI* SetThreadDescription)(HANDLE hThread,
    PCWSTR lpThreadDescription);

#endif

std::string getThreadName()
{
    std::string name;
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
    return name;
}

std::string setThreadName(const std::string &name)
{
    auto old = getThreadName();

#ifdef _WIN32
    // The SetThreadDescription API works even if no debugger is attached.
    auto set_thread_description_func =
        reinterpret_cast<SetThreadDescription>(::GetProcAddress(
            ::GetModuleHandleW(L"Kernel32.dll"), "SetThreadDescription"));
    if (set_thread_description_func) {
        set_thread_description_func(::GetCurrentThread(),
            to_wstring(name).c_str());
    }
#endif

    return old;
}

std::string appendThreadName(const std::string &name)
{
    return setThreadName(getThreadName() + name);
}

ScopedThreadName::ScopedThreadName(const std::string &name, bool append)
    : dbg(true /*primitives::isDebuggerAttached()*/)
{
    if (dbg)
        old_thread_name = append ? appendThreadName(name) : setThreadName(name);
}

ScopedThreadName::~ScopedThreadName()
{
    if (dbg)
        setThreadName(old_thread_name);
}

}
