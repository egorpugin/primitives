#define SW_MAIN_IMPL
#include <primitives/sw/main.h>

#include <primitives/sw/settings.h>
#include <primitives/main.h>

int main(int argc, char *argv[])
{
    // init settings early
    // e.g. before parsing command line
    getSettings();
    return SW_MAIN(argc, argv);
}
