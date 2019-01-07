// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <exception>
#include <string>

#define SW_BASE_EXCEPTION(t, msg) t(__FILE__, __func__, __LINE__, msg)
#define SW_EXCEPTION(t, msg) SW_BASE_EXCEPTION(sw::Exception, msg)
#define SW_RUNTIME_ERROR(msg) SW_BASE_EXCEPTION(sw::RuntimeError, msg)

namespace sw
{

struct Exception : std::exception
{
    Exception(const char* file, const char* function, int line, const std::string &msg);

    const char* what() const noexcept override;

protected:
    //std::string ex_type; // ?
    std::string file;
    std::string function;
    int line;
    std::string message;
    mutable std::string what_;

    std::string format() const;
};

struct RuntimeError : Exception
{
    using Exception::Exception;
};

}
