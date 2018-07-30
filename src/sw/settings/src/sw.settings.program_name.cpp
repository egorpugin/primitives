#include "primitives/sw/settings.h"

#if defined(CPPAN_EXECUTABLE)
String getProgramName()
{
    return PACKAGE_NAME_CLEAN;
}

String getVersionString()
{
    String s;
    s += getProgramName();
    s += " version ";
    s += PACKAGE_VERSION;
    return s;
}
#endif
