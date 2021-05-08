#include <primitives/sw/settings_program_name.h>

#if defined(SW_EXECUTABLE) && !defined(SW_CUSTOM_PROGRAM_NAME)

// this will trigger build error if executable forget to set
// PackageDefinitions = true;
#ifndef PACKAGE_NAME_CLEAN
#error "Set '.PackageDefinitions = true;' on your target."
#endif

EXPORT_FROM_EXECUTABLE
std::string getProgramName()
{
    return PACKAGE_NAME_CLEAN;
}

// use gitrev instead?
EXPORT_FROM_EXECUTABLE
std::string getVersionString()
{
    std::string s;
    s += ::sw::getProgramName();
    s += " version ";
    s += PACKAGE_VERSION;
    s += "\n";
    s += "assembled " __DATE__ " " __TIME__;
    return s;
}

#endif
