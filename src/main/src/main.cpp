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
    error = 0; // clear flag to prevent double break
}

int main(int argc, char *argv[])
{
    atexit(exit_handler);
    int ret = 2;
    try
    {
        ret = error = PRIMITIVES_MAIN(argc, argv);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
    exit_handler();
    return ret;
}
