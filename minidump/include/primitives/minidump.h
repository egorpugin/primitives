#pragma once

#include <string>

#ifdef _WIN32

#define PRIMITIVES_GENERATE_DUMP primitives::minidump::GenerateDump(GetExceptionInformation())

struct _EXCEPTION_POINTERS;

namespace primitives::minidump
{

extern std::wstring dir;
extern int v_major;
extern int v_minor;
extern int v_patch;

int GenerateDump(_EXCEPTION_POINTERS* pExceptionPointers);


}

#endif
