#pragma once

#include <primitives/filesystem.h>

namespace primitives
{

inline void *getCurrentModuleSymbol()
{
    return (void *)&getCurrentModuleSymbol;
}

PRIMITIVES_SYMBOL_API
void *getModuleForSymbol(void *f = nullptr);

PRIMITIVES_SYMBOL_API
path getModuleNameForSymbol(void *f = nullptr);

}
