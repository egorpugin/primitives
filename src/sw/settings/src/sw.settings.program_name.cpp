#include <string>

#if defined(CPPAN_EXECUTABLE)
namespace sw
{

std::string getProgramName()
{
    return PACKAGE_NAME_CLEAN;
}

}

namespace primitives
{

std::string getVersionString()
{
    std::string s;
    s += ::sw::getProgramName();
    s += " version ";
    s += PACKAGE_VERSION;
    return s;
}

}
#endif
