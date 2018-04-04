// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
