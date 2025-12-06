#pragma once

#include <primitives/filesystem.h>

#include <boost/dll.hpp>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifndef _WIN32
#define _GNU_SOURCE // for dladdr
#include <dlfcn.h>
#endif

namespace primitives
{

inline void *getCurrentModuleSymbol()
{
    return (void *)&getCurrentModuleSymbol;
}

inline void *getModuleForSymbol(void *f = nullptr)
{
#ifdef _WIN32
    HMODULE hModule = NULL;
    // hModule is NULL if GetModuleHandleEx fails.
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
        | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCTSTR)(f ? f : getCurrentModuleSymbol()), &hModule);
    return hModule;
#else
    Dl_info i;
    if (dladdr(f ? f : getCurrentModuleSymbol(), &i)) {
        return i.dli_fbase;
    }
    return nullptr;
#endif
}

#if !defined(_WIN32)
inline auto read_argv0() {
    String s;
    if (FILE *fp = fopen("/proc/self/cmdline", "r")) {
        char cmdline[8192]{};
        fgets(cmdline, sizeof cmdline, fp);
        fclose(fp);
        if (cmdline[0]) {
            s = cmdline;
        }
    }
    return s;
}
inline const String &read_current_argv0() {
    static const String argv0 = read_argv0();
    return argv0;
}
#endif

inline path getModuleNameForSymbol(void *f = nullptr)
{
#ifdef _WIN32
    auto lib = getModuleForSymbol(f);
    const auto sz = 1 << 16;
    WCHAR n[sz] = { 0 };
    GetModuleFileNameW((HMODULE)lib, n, sz);
    path m = n;
    return m;// .filename();
#else
    // on linux i.dli_fname is not full path, but rather argv[0]
    // fs::absolute also does not give us correct path
    if (!f) {
        return boost::dll::program_location().string();
    }
    Dl_info i;
    if (dladdr(f ? f : getCurrentModuleSymbol(), &i)) {
        if (strcmp(i.dli_fname, read_current_argv0().c_str()) == 0) { // check if equal to argv[0]
            return boost::dll::program_location().string();
        }
        return fs::absolute(i.dli_fname);
    }
    return {};
#endif
}

}
