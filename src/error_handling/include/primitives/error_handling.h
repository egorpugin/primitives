// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <iostream>
#include <string>

#ifdef _MSC_VER
#include <Windows.h>
#endif

inline void debug_break_if_debugger_attached()
{
#ifdef _MSC_VER
#ifndef NDEBUG
    if (IsDebuggerPresent())
        DebugBreak();
#endif
#endif
}

inline void report_fatal_error(const std::string &s)
{
    std::cerr << "fatal error: " << s << "!" << std::endl;
    exit(1);
}

namespace sw
{

/// This function calls abort(), and prints the optional message to stderr.
/// Use the llvm_unreachable macro (that adds location info), instead of
/// calling this function directly.
inline void unreachable_internal(const char *msg = nullptr, const char *file = nullptr, unsigned line = 0)
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

/// Marks that the current location is not supposed to be reachable.
/// In !NDEBUG builds, prints the message and location info to stderr.
/// In NDEBUG builds, becomes an optimizer hint that the current location
/// is not supposed to be reachable.  On compilers that don't support
/// such hints, prints a reduced message instead.
///
/// Use this instead of assert(0).  It conveys intent more clearly and
/// allows compilers to omit some unnecessary code.
#ifndef NDEBUG
#define sw_unreachable(msg) \
  ::sw::unreachable_internal(msg, __FILE__, __LINE__)
#else
#define sw_unreachable(msg) ::sw::unreachable_internal()
#endif
