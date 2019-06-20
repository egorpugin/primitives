#include <primitives/sw/settings_program_name.h>

#if defined(CPPAN_EXECUTABLE) || defined(SW_EXECUTABLE)
EXPORT_FROM_EXECUTABLE
std::string getProgramName()
{
    return PACKAGE_NAME_CLEAN;
}

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
