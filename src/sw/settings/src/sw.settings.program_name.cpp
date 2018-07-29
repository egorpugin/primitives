#include "primitives/sw/settings.h"

#if defined(CPPAN_EXECUTABLE) && !defined(CPPAN_SHARED_BUILD)
String getProgramName()
{
    return PACKAGE_NAME_CLEAN;
}
#endif
