#include <primitives/symbol.h>

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

void *getModuleForSymbol(void *f)
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
    if (dladdr(f ? f : getCurrentModuleSymbol(), &i))
        return i.dli_fbase;
    return nullptr;
#endif
}

path getModuleNameForSymbol(void *f)
{
#ifdef _WIN32
    auto lib = getModuleForSymbol(f);
    const auto sz = 1 << 16;
    WCHAR n[sz] = { 0 };
    GetModuleFileNameW((HMODULE)lib, n, sz);
    path m = n;
    return m;// .filename();
#else
    // on linux i.dli_fname is not full path
    // fs::absolute also does not give us correct path
    if (!f)
        return boost::dll::program_location().string();
    Dl_info i;
    if (dladdr(f ? f : getCurrentModuleSymbol(), &i))
        return fs::absolute(i.dli_fname);
    return {};
#endif
}

}
