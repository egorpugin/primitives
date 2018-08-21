#include "primitives/sw/settings.h"

#if defined(CPPAN_EXECUTABLE)
namespace sw
{

String getProgramName()
{
    return PACKAGE_NAME_CLEAN;
}

}

namespace primitives
{

String getVersionString()
{
    String s;
    s += ::sw::getProgramName();
    s += " version ";
    s += PACKAGE_VERSION;
    return s;
}

}
#endif
