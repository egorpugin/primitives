#define PRIMITIVES_MAIN_IMPL
#include <primitives/main.h>

#include <primitives/error_handling.h>

#include <iostream>
#include <exception>

#ifdef _WIN32
#include <Windows.h>
#endif

static int error = 1;
bool gPauseOnError;

void exit_handler()
{
    if (error == 0)
        return;
    if (gPauseOnError)
    {
        std::cout << "Stopped on error as you asked. Press any key to continue...";
        getchar();
    }
    else
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

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    return main(__argc, __argv);
}
#endif
