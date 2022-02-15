// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "debug.h"

#if !defined(__clang__) // until v14?
#include <source_location>
#endif
#include <stdexcept>
#include <string>

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

namespace detail
{

struct PRIMITIVES_TEMPLATES_API BaseException
{
    BaseException(const char *file, const char *function, int line, const std::string &msg, bool stacktrace);

    const char *getMessage() const;

protected:
    //std::string ex_type; // ?
    std::string file;
    std::string function;
    int line;
    std::string message;
    mutable std::string what_;

    std::string format() const;
};

}

struct PRIMITIVES_TEMPLATES_API Exception : detail::BaseException, /*virtual*/ std::exception
{
    Exception(const char *file, const char *function, int line, const std::string &msg, bool stacktrace);
};

struct PRIMITIVES_TEMPLATES_API RuntimeError : detail::BaseException, /*Exception, virtual*/ std::runtime_error
{
    RuntimeError(const char *file, const char *function, int line, const std::string &msg, bool stacktrace);
    //RuntimeError(const RuntimeError &);
#if !defined(__clang__) // until v14?
    RuntimeError(const std::string &msg, std::source_location = {}, bool stacktrace = true);
#endif
};
using runtime_error = RuntimeError;

struct PRIMITIVES_TEMPLATES_API LogicError : detail::BaseException, std::logic_error
{
    LogicError(const char *file, const char *function, int line, const std::string &msg, bool stacktrace);
};

}
