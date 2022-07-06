// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "debug.h"

#include <boost/stacktrace.hpp>

#include <chrono>
#include <iostream>
#include <sstream>
#if !defined(__clang__) // until v14?
#include <source_location>
#endif
#include <stdexcept>
#include <string>
#include <thread>

/*#ifdef _WIN32
#include <Windows.h>
#include <processthreadsapi.h>
#endif*/

#ifdef SW_EXCEPTIONS_NO_SOURCE_INFO
#define SW_BASE_EXCEPTION(t, msg, st) t("", "", 0, msg, st)
#else
#define SW_BASE_EXCEPTION(t, msg, st) t(__FILE__, __func__, __LINE__, msg, st)
#endif

#define SW_EXCEPTION_CUSTOM(msg, st) SW_BASE_EXCEPTION(sw::Exception, msg, st)
#define SW_RUNTIME_ERROR_CUSTOM(msg, st) SW_BASE_EXCEPTION(sw::RuntimeError, msg, st)
#define SW_LOGIC_ERROR_CUSTOM(msg, st) SW_BASE_EXCEPTION(sw::LogicError, msg, st)

#define SW_EXCEPTION(msg) SW_EXCEPTION_CUSTOM(msg, true)
#define SW_RUNTIME_ERROR(msg) SW_RUNTIME_ERROR_CUSTOM(msg, true)
#define SW_LOGIC_ERROR(msg) SW_LOGIC_ERROR_CUSTOM(msg, true)

#define SW_ASSERT(x, msg) do { if (!(x)) throw SW_RUNTIME_ERROR(msg); } while (0)
#define SW_CHECK(x) SW_ASSERT(x, #x)

#define SW_UNIMPLEMENTED                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        DEBUG_BREAK;                                                                                                   \
        throw SW_RUNTIME_ERROR("TODO: unimplemented");                                                                 \
    } while (0)

#define SW_UNREACHABLE                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        DEBUG_BREAK;                                                                                                   \
        throw SW_LOGIC_ERROR("unreachable code");                                                                      \
    } while (0)

namespace sw
{

using namespace std::chrono_literals;

inline bool gUseStackTrace;
inline bool gDebugOnException;
inline std::string gSymbolPath;

#ifdef BOOST_MSVC
#include "exceptions_msvc.h"
#endif

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

namespace detail
{

struct BaseException
{
    BaseException(const char *file, const char *function, int line, const std::string &msg, bool stacktrace)
        : file(file), function(function), line(line), message(msg)
    {
        if (stacktrace && gUseStackTrace)
        {
            // skip 3 frames
            // -1 means till the end
            boost::stacktrace::stacktrace t(3, -1);
            message += "\nStacktrace:\n";
    #ifdef BOOST_MSVC
            message += ::sw::to_string(t);
    #else
            std::ostringstream ss;
            ss << t;
            message += ss.str();
    #endif
        }
    }

    const char *getMessage() const
    {
        if (what_.empty())
            what_ = format();
        return what_.c_str();
    }

protected:
    //std::string ex_type; // ?
    std::string file;
    std::string function;
    int line;
    std::string message;
    mutable std::string what_;

    std::string format() const
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
};

} // namespace detail

struct Exception : detail::BaseException, /*virtual*/ std::exception
{
    Exception(const char *file, const char *function, int line, const std::string &msg, bool stacktrace)
        : BaseException(file, function, line, msg, stacktrace), std::exception(
    #ifdef _MSC_VER
            getMessage()
    #endif
        )
    {
    }
};

struct RuntimeError : detail::BaseException, /*Exception, virtual*/ std::runtime_error
{
    RuntimeError(const char *file, const char *function, int line, const std::string &msg, bool stacktrace)
        : BaseException
        //Exception
        (file, function, line, msg, stacktrace), std::runtime_error(getMessage())
    {
        doe(getMessage());
    }

    //RuntimeError(const RuntimeError &);
#if !defined(__clang__) // until v14?
    RuntimeError(const std::string &msg, std::source_location loc = {}, bool stacktrace = true)
        : BaseException
        //Exception
        (loc.file_name(), loc.function_name(), loc.line(), msg, stacktrace), std::runtime_error(getMessage())
    {
        doe(getMessage());
    }
#endif
};
using runtime_error = RuntimeError;

struct LogicError : detail::BaseException, std::logic_error
{
    LogicError(const char *file, const char *function, int line, const std::string &msg, bool stacktrace)
        : BaseException
        //Exception
        (file, function, line, msg, stacktrace), std::logic_error(getMessage())
    {
        doe(getMessage());
    }
};
using logic_error = LogicError;

}
