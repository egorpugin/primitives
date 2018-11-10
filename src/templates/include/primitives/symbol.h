#pragma once

namespace primitives
{

inline void *getCurrentModuleSymbol()
{
    return (void *)&getCurrentModuleSymbol;
}

void *getModuleForSymbol(void *f = nullptr);

}
