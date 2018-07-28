// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/error_handling.h>

#ifdef _MSC_VER
#include <Windows.h>
#endif

void debug_break_if_debugger_attached()
{
#ifdef _MSC_VER
    if (IsDebuggerPresent())
        DebugBreak();
#endif
}

void report_fatal_error(const std::string &s)
{
    debug_break_if_debugger_attached();
    exit(1);
}
