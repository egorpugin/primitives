// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <stdexcept>
#include <string>

#define SW_BASE_EXCEPTION(t, msg, st) t(__FILE__, __func__, __LINE__, msg, st)

#define SW_EXCEPTION_CUSTOM(msg, st) SW_BASE_EXCEPTION(sw::Exception, msg, st)
#define SW_RUNTIME_ERROR_CUSTOM(msg, st) SW_BASE_EXCEPTION(sw::RuntimeError, msg, st)
#define SW_LOGIC_ERROR_CUSTOM(msg, st) SW_BASE_EXCEPTION(sw::LogicError, msg, st)

#define SW_EXCEPTION(msg) SW_EXCEPTION_CUSTOM(msg, true)
#define SW_RUNTIME_ERROR(msg) SW_RUNTIME_ERROR_CUSTOM(msg, true)
#define SW_LOGIC_ERROR(msg) SW_LOGIC_ERROR_CUSTOM(msg, true)

#define SW_UNIMPLEMENTED throw SW_RUNTIME_ERROR("TODO: unimplemented")

namespace sw
{

namespace detail
{

struct BaseException
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

struct Exception : detail::BaseException, /*virtual*/ std::exception
{
    Exception(const char *file, const char *function, int line, const std::string &msg, bool stacktrace);
};

struct RuntimeError : detail::BaseException, /*Exception, virtual*/ std::runtime_error
{
    RuntimeError(const char *file, const char *function, int line, const std::string &msg, bool stacktrace);
    //RuntimeError(const RuntimeError &);
};

struct LogicError : detail::BaseException, std::logic_error
{
    LogicError(const char *file, const char *function, int line, const std::string &msg, bool stacktrace);
};

}
