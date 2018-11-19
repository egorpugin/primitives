#include <primitives/templates.h>

#include <iostream>

ScopeGuard::~ScopeGuard()
{
    if (!active)
        return;

    // handle not in run, but here in dtor
    try
    {
        run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "exception was thrown in scope guard: " << e.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "unknown exception was thrown in scope guard\n";
    }
}

void ScopeGuard::run()
{
    if (!f)
        return;

    if (flag)
        std::call_once(*flag, f);
    else
        f();
}
