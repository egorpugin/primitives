// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/exceptions.h>

namespace sw
{

Exception::Exception(const char* file, const char* function, int line, const std::string &msg)
    : file(file), function(function), line(line), message(msg)
{
}

const char* Exception::what() const
{
    if (what_.empty())
        what_ = format();
    return what_.c_str();
}

std::string Exception::format() const
{
    std::string s;
    //s += ex_type;
    s += "Exception in file " + file + ":" + std::to_string(line) + ", function " + function + ": " + message;
    return s;
}

}
