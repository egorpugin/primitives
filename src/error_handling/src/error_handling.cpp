// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/error_handling.h>

#include <iostream>

#ifdef _MSC_VER
#include <Windows.h>
#endif

void debug_break_if_debugger_attached()
{
#ifdef _MSC_VER
#ifndef NDEBUG
    if (IsDebuggerPresent())
        DebugBreak();
#endif
#endif
}

void report_fatal_error(const std::string &s)
{
    std::cerr << "fatal error: " << s << "!" << std::endl;
    exit(1);
}

namespace sw
{

void unreachable_internal(const char *msg, const char *file, unsigned line)
{
    // This code intentionally doesn't call the ErrorHandler callback, because
    // llvm_unreachable is intended to be used to indicate "impossible"
    // situations, and not legitimate runtime errors.
    if (msg)
        std::cerr << msg << "\n";
    std::cerr << "UNREACHABLE executed";
    if (file)
        std::cerr << " at " << file << ":" << line;
    std::cerr << "!\n";
    abort();
}

}
