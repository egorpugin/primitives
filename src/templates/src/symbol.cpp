#include <primitives/symbol.h>

#ifdef _WIN32
#include <Windows.h>
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

}
