#define PRIMITIVES_MAIN_IMPL
#include "primitives/main.h"

#include <iostream>
#include <exception>

#ifdef _WIN32
#include <Windows.h>
#endif

int main1(int argc, char *argv[])
{
    try
    {
        return PRIMITIVES_MAIN(argc, argv);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "unknown exception" << std::endl;
    }
#ifdef _MSC_VER
    if (IsDebuggerPresent())
        DebugBreak();
#endif
    return 1;
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
    __try
    {
#endif
        return main1(argc, argv);
#ifdef _WIN32
    }
    //__except (EXCEPTION_CONTINUE_SEARCH)
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        std::cerr << "unknown SEH exception" << std::endl;
        return 1;
    }
#endif
}
