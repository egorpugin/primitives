// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "debug.h"

#include <source_location>
#include <stdexcept>
#include <string>

#define SW_BASE_EXCEPTION(t, msg, st) t(msg, st)

#define SW_EXCEPTION_CUSTOM(msg, st) SW_BASE_EXCEPTION(sw::Exception, msg, st)
#define SW_RUNTIME_ERROR_CUSTOM(msg, st) SW_BASE_EXCEPTION(sw::RuntimeError, msg, st)
#define SW_LOGIC_ERROR_CUSTOM(msg, st) SW_BASE_EXCEPTION(sw::LogicError, msg, st)

#define SW_EXCEPTION(msg) SW_EXCEPTION_CUSTOM(msg)
#define SW_RUNTIME_ERROR(msg) SW_RUNTIME_ERROR_CUSTOM(msg)
#define SW_LOGIC_ERROR(msg) SW_LOGIC_ERROR_CUSTOM(msg)

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

namespace sw {

namespace detail {

struct PRIMITIVES_TEMPLATES_API BaseException {
    BaseException(const std::string &msg, bool stacktrace = true, const std::source_location & = {});

    const char *getMessage() const;

protected:
    //std::string ex_type; // ?
    std::source_location loc;
    std::string message;
    mutable std::string what_;

    std::string format() const;
};

} // namespace detail

struct PRIMITIVES_TEMPLATES_API Exception : detail::BaseException, /*virtual*/ std::exception {
    Exception(const std::string &msg, bool stacktrace = true, const std::source_location & = {});
};

struct PRIMITIVES_TEMPLATES_API runtime_error : detail::BaseException, /*Exception, virtual*/ std::runtime_error {
    runtime_error(const std::string &msg, bool stacktrace = true, const std::source_location & = {});
};
using RuntimeError = runtime_error;

struct PRIMITIVES_TEMPLATES_API LogicError : detail::BaseException, std::logic_error {
    LogicError(const std::string &msg, bool stacktrace = true, const std::source_location & = {});
};

} // namespace sw
