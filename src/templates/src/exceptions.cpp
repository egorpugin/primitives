// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/exceptions.h>
#include <boost/stacktrace.hpp>

#include <primitives/debug.h>

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

bool gUseStackTrace;
bool gDebugOnException;
std::string gSymbolPath;

#ifdef BOOST_MSVC
#include "exceptions_msvc.h"
#endif

using namespace std::chrono_literals;

namespace sw
{

namespace detail
{

BaseException::BaseException(const char *file, const char *function, int line, const std::string &msg, bool stacktrace)
    : file(file), function(function), line(line), message(msg)
{
    if (stacktrace && gUseStackTrace)
    {
        // skip 3 frames
        // -1 means till the end
        boost::stacktrace::stacktrace t(3, -1);
        message += "\nStacktrace:\n";
#ifdef BOOST_MSVC
        message += ::to_string(t);
#else
        std::ostringstream ss;
        ss << t;
        message += ss.str();
#endif
    }
}

const char *BaseException::getMessage() const
{
    if (what_.empty())
        what_ = format();
    return what_.c_str();
}

std::string BaseException::format() const
{
    std::string s;
    //s += ex_type;
    //s += "Exception: " + message; // disabled for now, too poor :(
    if (file.empty() && function.empty() && line == 0)
        s += "Exception: " + message;
    else
        s += "Exception in file " + file + ":" + std::to_string(line) + ", function " + function + ": " + message;
    return s;
}

}

Exception::Exception(const char *file, const char *function, int line, const std::string &msg, bool stacktrace)
    : BaseException(file, function, line, msg, stacktrace), std::exception(
#ifdef _MSC_VER
        getMessage()
#endif
    )
{
}

static void doe(const std::string &msg)
{
    if (!gDebugOnException)
        return;
    if (primitives::isDebuggerAttached())
        return;

    std::cerr << msg << "\n";
    std::cerr << "Waiting for debugger..." << "\n";
    while (!primitives::isDebuggerAttached())
        std::this_thread::sleep_for(100ms);
}

RuntimeError::RuntimeError(const char *file, const char *function, int line, const std::string &msg, bool stacktrace)
    : BaseException
    //Exception
    (file, function, line, msg, stacktrace), std::runtime_error(getMessage())
{
    doe(getMessage());
}

#if !defined(__clang__) // until v14?
RuntimeError::RuntimeError(const std::string &msg, std::source_location loc, bool stacktrace)
    : BaseException
    //Exception
    (loc.file_name(), loc.function_name(), loc.line(), msg, stacktrace), std::runtime_error(getMessage())
{
    doe(getMessage());
}
#endif

LogicError::LogicError(const char *file, const char *function, int line, const std::string &msg, bool stacktrace)
    : BaseException
    //Exception
    (file, function, line, msg, stacktrace), std::logic_error(getMessage())
{
    doe(getMessage());
}

} // namespace sw
