#include "primitives/sw/settings.h"

#if defined(CPPAN_EXECUTABLE)
String getProgramName()
{
    return PACKAGE_NAME_CLEAN;
}
#endif
