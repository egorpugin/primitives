#define PRIMITIVES_MAIN_IMPL
#include <primitives/main.h>

#include <primitives/error_handling.h>

#include <iostream>
#include <exception>

#ifdef _WIN32
#include <Windows.h>
#endif

static int error = 1;

void exit_handler()
{
    if (error)
        debug_break_if_debugger_attached();
}

int main1(int argc, char *argv[])
{
    atexit(exit_handler);
    try
    {
        return error = PRIMITIVES_MAIN(argc, argv);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "unknown exception" << std::endl;
    }
    return 2;
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
    }
#endif
    return 1;
}
